// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Defined by build system; this avoids IDE warnings. Must come before
// color_management.h (affects header definitions).
#ifndef JPEGXL_ENABLE_SKCMS
#define JPEGXL_ENABLE_SKCMS 0
#endif

#include "lib/jxl/color_management.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <utility>

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "lib/jxl/color_management.cc"
#include <hwy/foreach_target.h>
#include <hwy/highway.h>

#include "lib/jxl/base/compiler_specific.h"
#include "lib/jxl/base/data_parallel.h"
#include "lib/jxl/base/status.h"
#include "lib/jxl/field_encodings.h"
#include "lib/jxl/linalg.h"  // MatMul, Inv3x3Matrix
#include "lib/jxl/transfer_functions-inl.h"

HWY_BEFORE_NAMESPACE();
namespace jxl {
namespace HWY_NAMESPACE {

// NOTE: this is only used to provide a reasonable ICC profile that other
// software can read. Our own transforms use ExtraTF instead because that is
// more precise and supports unbounded mode.
std::vector<uint16_t> CreateTableCurve(uint32_t N, const ExtraTF tf) {
  JXL_ASSERT(N <= 4096);  // ICC MFT2 only allows 4K entries
  JXL_ASSERT(tf == ExtraTF::kPQ || tf == ExtraTF::kHLG);
  // No point using float - LCMS converts to 16-bit for A2B/MFT.
  std::vector<uint16_t> table(N);
  for (uint32_t i = 0; i < N; ++i) {
    const float x = static_cast<float>(i) / (N - 1);  // 1.0 at index N - 1.
    const double dx = static_cast<double>(x);
    // LCMS requires EOTF (e.g. 2.4 exponent).
    double y = (tf == ExtraTF::kHLG) ? TF_HLG().DisplayFromEncoded(dx)
                                     : TF_PQ().DisplayFromEncoded(dx);
    JXL_ASSERT(y >= 0.0);
    // Clamp to table range - necessary for HLG.
    if (y > 1.0) y = 1.0;
    // 1.0 corresponds to table value 0xFFFF.
    table[i] = static_cast<uint16_t>(roundf(y * 65535.0));
  }
  return table;
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace jxl
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace jxl {

HWY_EXPORT(CreateTableCurve);  // Local function.

Status CIEXYZFromWhiteCIExy(const CIExy& xy, float XYZ[3]) {
  // Target Y = 1.
  if (std::abs(xy.y) < 1e-12) return JXL_FAILURE("Y value is too small");
  const float factor = 1 / xy.y;
  XYZ[0] = xy.x * factor;
  XYZ[1] = 1;
  XYZ[2] = (1 - xy.x - xy.y) * factor;
  return true;
}

namespace {

// NOTE: this is only used to provide a reasonable ICC profile that other
// software can read. Our own transforms use ExtraTF instead because that is
// more precise and supports unbounded mode.
template <class Func>
std::vector<uint16_t> CreateTableCurve(uint32_t N, const Func& func) {
  JXL_ASSERT(N <= 4096);  // ICC MFT2 only allows 4K entries
  // No point using float - LCMS converts to 16-bit for A2B/MFT.
  std::vector<uint16_t> table(N);
  for (uint32_t i = 0; i < N; ++i) {
    const float x = static_cast<float>(i) / (N - 1);  // 1.0 at index N - 1.
    // LCMS requires EOTF (e.g. 2.4 exponent).
    double y = func.DisplayFromEncoded(static_cast<double>(x));
    JXL_ASSERT(y >= 0.0);
    // Clamp to table range - necessary for HLG.
    if (y > 1.0) y = 1.0;
    // 1.0 corresponds to table value 0xFFFF.
    table[i] = static_cast<uint16_t>(roundf(y * 65535.0));
  }
  return table;
}

Status CreateICCChadMatrix(CIExy w, float result[9]) {
  float m[9];
  if (w.y == 0) {  // WhitePoint can not be pitch-black.
    return JXL_FAILURE("Invalid WhitePoint");
  }
  JXL_RETURN_IF_ERROR(AdaptToXYZD50(w.x, w.y, m));
  memcpy(result, m, sizeof(float) * 9);
  return true;
}

// Creates RGB to XYZ matrix given RGB primaries and whitepoint in xy.
Status CreateICCRGBMatrix(CIExy r, CIExy g, CIExy b, CIExy w, float result[9]) {
  float m[9];
  JXL_RETURN_IF_ERROR(
      PrimariesToXYZD50(r.x, r.y, g.x, g.y, b.x, b.y, w.x, w.y, m));
  memcpy(result, m, sizeof(float) * 9);
  return true;
}

void WriteICCUint32(uint32_t value, size_t pos, PaddedBytes* JXL_RESTRICT icc) {
  if (icc->size() < pos + 4) icc->resize(pos + 4);
  (*icc)[pos + 0] = (value >> 24u) & 255;
  (*icc)[pos + 1] = (value >> 16u) & 255;
  (*icc)[pos + 2] = (value >> 8u) & 255;
  (*icc)[pos + 3] = value & 255;
}

void WriteICCUint16(uint16_t value, size_t pos, PaddedBytes* JXL_RESTRICT icc) {
  if (icc->size() < pos + 2) icc->resize(pos + 2);
  (*icc)[pos + 0] = (value >> 8u) & 255;
  (*icc)[pos + 1] = value & 255;
}

// Writes a 4-character tag
void WriteICCTag(const char* value, size_t pos, PaddedBytes* JXL_RESTRICT icc) {
  if (icc->size() < pos + 4) icc->resize(pos + 4);
  memcpy(icc->data() + pos, value, 4);
}

Status WriteICCS15Fixed16(float value, size_t pos,
                          PaddedBytes* JXL_RESTRICT icc) {
  // "nextafterf" for 32768.0f towards zero are:
  // 32767.998046875, 32767.99609375, 32767.994140625
  // Even the first value works well,...
  bool ok = (-32767.995f <= value) && (value <= 32767.995f);
  if (!ok) return JXL_FAILURE("ICC value is out of range / NaN");
  int32_t i = value * 65536.0f + 0.5f;
  // Use two's complement
  uint32_t u = static_cast<uint32_t>(i);
  WriteICCUint32(u, pos, icc);
  return true;
}

Status CreateICCHeader(const ColorEncoding& c,
                       PaddedBytes* JXL_RESTRICT header) {
  // TODO(lode): choose color management engine name, e.g. "skia" if
  // integrated in skia.
  static const char* kCmm = "jxl ";

  header->resize(128, 0);

  WriteICCUint32(0, 0, header);  // size, correct value filled in at end
  WriteICCTag(kCmm, 4, header);
  WriteICCUint32(0x04300000u, 8, header);
  WriteICCTag("mntr", 12, header);
  WriteICCTag(c.IsGray() ? "GRAY" : "RGB ", 16, header);
  WriteICCTag("XYZ ", 20, header);

  // Three uint32_t's date/time encoding.
  // TODO(lode): encode actual date and time, this is a placeholder
  uint32_t year = 2019, month = 12, day = 1;
  uint32_t hour = 0, minute = 0, second = 0;
  WriteICCUint16(year, 24, header);
  WriteICCUint16(month, 26, header);
  WriteICCUint16(day, 28, header);
  WriteICCUint16(hour, 30, header);
  WriteICCUint16(minute, 32, header);
  WriteICCUint16(second, 34, header);

  WriteICCTag("acsp", 36, header);
  WriteICCTag("APPL", 40, header);
  WriteICCUint32(0, 44, header);  // flags
  WriteICCUint32(0, 48, header);  // device manufacturer
  WriteICCUint32(0, 52, header);  // device model
  WriteICCUint32(0, 56, header);  // device attributes
  WriteICCUint32(0, 60, header);  // device attributes
  WriteICCUint32(static_cast<uint32_t>(c.rendering_intent), 64, header);

  // Mandatory D50 white point of profile connection space
  WriteICCUint32(0x0000f6d6, 68, header);
  WriteICCUint32(0x00010000, 72, header);
  WriteICCUint32(0x0000d32d, 76, header);

  WriteICCTag(kCmm, 80, header);

  return true;
}

void AddToICCTagTable(const char* tag, size_t offset, size_t size,
                      PaddedBytes* JXL_RESTRICT tagtable,
                      std::vector<size_t>* offsets) {
  WriteICCTag(tag, tagtable->size(), tagtable);
  // writing true offset deferred to later
  WriteICCUint32(0, tagtable->size(), tagtable);
  offsets->push_back(offset);
  WriteICCUint32(size, tagtable->size(), tagtable);
}

void FinalizeICCTag(PaddedBytes* JXL_RESTRICT tags, size_t* offset,
                    size_t* size) {
  while ((tags->size() & 3) != 0) {
    tags->push_back(0);
  }
  *offset += *size;
  *size = tags->size() - *offset;
}

// The input text must be ASCII, writing other characters to UTF-16 is not
// implemented.
void CreateICCMlucTag(const std::string& text, PaddedBytes* JXL_RESTRICT tags) {
  WriteICCTag("mluc", tags->size(), tags);
  WriteICCUint32(0, tags->size(), tags);
  WriteICCUint32(1, tags->size(), tags);
  WriteICCUint32(12, tags->size(), tags);
  WriteICCTag("enUS", tags->size(), tags);
  WriteICCUint32(text.size() * 2, tags->size(), tags);
  WriteICCUint32(28, tags->size(), tags);
  for (size_t i = 0; i < text.size(); i++) {
    tags->push_back(0);  // prepend 0 for UTF-16
    tags->push_back(text[i]);
  }
}

Status CreateICCXYZTag(float xyz[3], PaddedBytes* JXL_RESTRICT tags) {
  WriteICCTag("XYZ ", tags->size(), tags);
  WriteICCUint32(0, tags->size(), tags);
  for (size_t i = 0; i < 3; ++i) {
    JXL_RETURN_IF_ERROR(WriteICCS15Fixed16(xyz[i], tags->size(), tags));
  }
  return true;
}

Status CreateICCChadTag(float chad[9], PaddedBytes* JXL_RESTRICT tags) {
  WriteICCTag("sf32", tags->size(), tags);
  WriteICCUint32(0, tags->size(), tags);
  for (size_t i = 0; i < 9; i++) {
    JXL_RETURN_IF_ERROR(WriteICCS15Fixed16(chad[i], tags->size(), tags));
  }
  return true;
}

void CreateICCCurvCurvTag(const std::vector<uint16_t>& curve,
                          PaddedBytes* JXL_RESTRICT tags) {
  size_t pos = tags->size();
  tags->resize(tags->size() + 12 + curve.size() * 2, 0);
  WriteICCTag("curv", pos, tags);
  WriteICCUint32(0, pos + 4, tags);
  WriteICCUint32(curve.size(), pos + 8, tags);
  for (size_t i = 0; i < curve.size(); i++) {
    WriteICCUint16(curve[i], pos + 12 + i * 2, tags);
  }
}

Status CreateICCCurvParaTag(std::vector<float> params, size_t curve_type,
                            PaddedBytes* JXL_RESTRICT tags) {
  WriteICCTag("para", tags->size(), tags);
  WriteICCUint32(0, tags->size(), tags);
  WriteICCUint16(curve_type, tags->size(), tags);
  WriteICCUint16(0, tags->size(), tags);
  for (size_t i = 0; i < params.size(); i++) {
    JXL_RETURN_IF_ERROR(WriteICCS15Fixed16(params[i], tags->size(), tags));
  }
  return true;
}
}  // namespace

Status MaybeCreateProfile(const ColorEncoding& c,
                          PaddedBytes* JXL_RESTRICT icc) {
  PaddedBytes header, tagtable, tags;

  if (c.GetColorSpace() == ColorSpace::kUnknown || c.tf.IsUnknown()) {
    return false;  // Not an error
  }

  switch (c.GetColorSpace()) {
    case ColorSpace::kRGB:
    case ColorSpace::kGray:
      break;  // OK
    case ColorSpace::kXYB:
      return JXL_FAILURE("XYB ICC not yet implemented");
    default:
      return JXL_FAILURE("Invalid CS %u",
                         static_cast<unsigned int>(c.GetColorSpace()));
  }

  JXL_RETURN_IF_ERROR(CreateICCHeader(c, &header));

  std::vector<size_t> offsets;
  // tag count, deferred to later
  WriteICCUint32(0, tagtable.size(), &tagtable);

  size_t tag_offset = 0, tag_size = 0;

  CreateICCMlucTag(Description(c), &tags);
  FinalizeICCTag(&tags, &tag_offset, &tag_size);
  AddToICCTagTable("desc", tag_offset, tag_size, &tagtable, &offsets);

  const std::string copyright =
      "Copyright 2019 Google LLC, CC-BY-SA 3.0 Unported "
      "license(https://creativecommons.org/licenses/by-sa/3.0/legalcode)";
  CreateICCMlucTag(copyright, &tags);
  FinalizeICCTag(&tags, &tag_offset, &tag_size);
  AddToICCTagTable("cprt", tag_offset, tag_size, &tagtable, &offsets);

  // TODO(eustas): isn't it the other way round: gray image has d50 WhitePoint?
  if (c.IsGray()) {
    float wtpt[3];
    JXL_RETURN_IF_ERROR(CIEXYZFromWhiteCIExy(c.GetWhitePoint(), wtpt));
    JXL_RETURN_IF_ERROR(CreateICCXYZTag(wtpt, &tags));
  } else {
    float d50[3] = {0.964203, 1.0, 0.824905};
    JXL_RETURN_IF_ERROR(CreateICCXYZTag(d50, &tags));
  }
  FinalizeICCTag(&tags, &tag_offset, &tag_size);
  AddToICCTagTable("wtpt", tag_offset, tag_size, &tagtable, &offsets);

  if (!c.IsGray()) {
    // Chromatic adaptation matrix
    float chad[9];
    JXL_RETURN_IF_ERROR(CreateICCChadMatrix(c.GetWhitePoint(), chad));

    const PrimariesCIExy primaries = c.GetPrimaries();
    float m[9];
    JXL_RETURN_IF_ERROR(CreateICCRGBMatrix(primaries.r, primaries.g,
                                           primaries.b, c.GetWhitePoint(), m));
    float r[3] = {m[0], m[3], m[6]};
    float g[3] = {m[1], m[4], m[7]};
    float b[3] = {m[2], m[5], m[8]};

    JXL_RETURN_IF_ERROR(CreateICCChadTag(chad, &tags));
    FinalizeICCTag(&tags, &tag_offset, &tag_size);
    AddToICCTagTable("chad", tag_offset, tag_size, &tagtable, &offsets);

    JXL_RETURN_IF_ERROR(CreateICCXYZTag(r, &tags));
    FinalizeICCTag(&tags, &tag_offset, &tag_size);
    AddToICCTagTable("rXYZ", tag_offset, tag_size, &tagtable, &offsets);

    JXL_RETURN_IF_ERROR(CreateICCXYZTag(g, &tags));
    FinalizeICCTag(&tags, &tag_offset, &tag_size);
    AddToICCTagTable("gXYZ", tag_offset, tag_size, &tagtable, &offsets);

    JXL_RETURN_IF_ERROR(CreateICCXYZTag(b, &tags));
    FinalizeICCTag(&tags, &tag_offset, &tag_size);
    AddToICCTagTable("bXYZ", tag_offset, tag_size, &tagtable, &offsets);
  }

  if (c.tf.IsGamma()) {
    float gamma = 1.0 / c.tf.GetGamma();
    JXL_RETURN_IF_ERROR(
        CreateICCCurvParaTag({gamma, 1.0, 0.0, 1.0, 0.0}, 3, &tags));
  } else {
    switch (c.tf.GetTransferFunction()) {
      case TransferFunction::kHLG:
        CreateICCCurvCurvTag(
            HWY_DYNAMIC_DISPATCH(CreateTableCurve)(4096, ExtraTF::kHLG), &tags);
        break;
      case TransferFunction::kPQ:
        CreateICCCurvCurvTag(
            HWY_DYNAMIC_DISPATCH(CreateTableCurve)(4096, ExtraTF::kPQ), &tags);
        break;
      case TransferFunction::kSRGB:
        JXL_RETURN_IF_ERROR(CreateICCCurvParaTag(
            {2.4, 1.0 / 1.055, 0.055 / 1.055, 1.0 / 12.92, 0.04045}, 3, &tags));
        break;
      case TransferFunction::k709:
        JXL_RETURN_IF_ERROR(CreateICCCurvParaTag(
            {1.0 / 0.45, 1.0 / 1.099, 0.099 / 1.099, 1.0 / 4.5, 0.081}, 3,
            &tags));
        break;
      case TransferFunction::kLinear:
        JXL_RETURN_IF_ERROR(
            CreateICCCurvParaTag({1.0, 1.0, 0.0, 1.0, 0.0}, 3, &tags));
        break;
      case TransferFunction::kDCI:
        JXL_RETURN_IF_ERROR(
            CreateICCCurvParaTag({2.6, 1.0, 0.0, 1.0, 0.0}, 3, &tags));
        break;
      default:
        JXL_ABORT("Unknown TF %d", c.tf.GetTransferFunction());
    }
  }
  FinalizeICCTag(&tags, &tag_offset, &tag_size);
  if (c.IsGray()) {
    AddToICCTagTable("kTRC", tag_offset, tag_size, &tagtable, &offsets);
  } else {
    AddToICCTagTable("rTRC", tag_offset, tag_size, &tagtable, &offsets);
    AddToICCTagTable("gTRC", tag_offset, tag_size, &tagtable, &offsets);
    AddToICCTagTable("bTRC", tag_offset, tag_size, &tagtable, &offsets);
  }

  // Tag count
  WriteICCUint32(offsets.size(), 0, &tagtable);
  for (size_t i = 0; i < offsets.size(); i++) {
    WriteICCUint32(offsets[i] + header.size() + tagtable.size(), 4 + 12 * i + 4,
                   &tagtable);
  }

  // ICC profile size
  WriteICCUint32(header.size() + tagtable.size() + tags.size(), 0, &header);

  *icc = header;
  icc->append(tagtable);
  icc->append(tags);

  // rendering intent, and region of the checksum itself, set to 0.
  // TODO(lode): manually verify with a reliable tool that this creates correct
  // signature (profile id) for ICC profiles.
  PaddedBytes icc_sum = *icc;
  memset(icc_sum.data() + 44, 0, 4);
  memset(icc_sum.data() + 64, 0, 4);

  return true;
}

}  // namespace jxl
#endif  // HWY_ONCE
