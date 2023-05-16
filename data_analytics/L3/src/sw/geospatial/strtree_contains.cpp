/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <omp.h>
#include "common.hpp"
#include "relation.hpp"
#include <arrow/csv/reader.h>
#include <arrow/csv/options.h>
#include "arrow/io/interfaces.h"
#include "arrow/buffer.h"
#include "arrow/table.h"
#include "arrow/array.h"
#include "arrow/io/memory.h"
#include "arrow/io/file.h"
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "strtree_contains.hpp"
#ifdef _STRTree_Kernel_
#include "strtree_kernel.hpp"
#endif
namespace xf {
namespace data_analytics {
namespace geospatial {

/**
 * @brief STRTree GeoSpatial Contains (point in polygon) based on STRTree
 * @tparam NC node capacity
 */
template <int NC>
class STRTree {
   private:
    char* _file_buf;
    uint32_t* _xys;
    int _real_sz, _index_sz;
    int _level = 0;
    std::shared_ptr<arrow::Table> table;
    std::shared_ptr<arrow::ChunkedArray> _xs, _ys;
    Node* _index;

    // round up
    int ceil(int s, int d) {
        int r = s / d;
        if (s % d) r++;
        return r;
    }

    int calcuSlice(int size) {
        int div = ceil(size, NC);
        return std::ceil(std::sqrt(div));
    }

    // calculate a node of upper layer
    void parentNode(Point* points, int addr, int size, Node& node) {
        node.xmin = points[0].x;
        node.xmax = points[0].x;
        node.ymin = points[0].y;
        node.ymax = points[0].y;
        for (int i = 1; i < size; i++) {
            if (node.xmin > points[i].x) node.xmin = points[i].x;
            if (node.xmax < points[i].x) node.xmax = points[i].x;
            if (node.ymin > points[i].y) node.ymin = points[i].y;
            if (node.ymax < points[i].y) node.ymax = points[i].y;
        }
        node.level = _level;
        node.level(15, 8) = size;
        node.addr = addr;
        // printNode(node);
    }

    // calculate nodes of upper layer
    void parentNodes(Point* points, int addr, int size, Node* o_nodes) {
        int l = ceil(size, NC);
        for (int i = 0; i < l; i++) {
            if (i == l - 1)
                parentNode(points + i * NC, addr + i * NC, size - i * NC, o_nodes[i]);
            else
                parentNode(points + i * NC, addr + i * NC, NC, o_nodes[i]);
        }
    }

    // calculate a node of upper layer
    void parentNode(Node* nodes, int addr, int size, Node& node) {
        node.xmin = nodes[0].xmin;
        node.xmax = nodes[0].xmax;
        node.ymin = nodes[0].ymin;
        node.ymax = nodes[0].ymax;
        for (int i = 1; i < size; i++) {
            if (node.xmin > nodes[i].xmin) node.xmin = nodes[i].xmin;
            if (node.xmax < nodes[i].xmax) node.xmax = nodes[i].xmax;
            if (node.ymin > nodes[i].ymin) node.ymin = nodes[i].ymin;
            if (node.ymax < nodes[i].ymax) node.ymax = nodes[i].ymax;
        }
        node.level = _level;
        node.level(15, 8) = size;
        node.addr = addr;
        // printNode(node);
    }

    // calculate nodes of a slice of upper layer
    void parentNodes(Node* i_nodes, int addr, int size, Node* o_nodes) {
        int l = ceil(size, NC);
        for (int i = 0; i < l; i++) {
            if (i == l - 1)
                parentNode(i_nodes + i * NC, addr + i * NC, size - i * NC, o_nodes[i]);
            else
                parentNode(i_nodes + i * NC, addr + i * NC, NC, o_nodes[i]);
        }
    }

    // calculate all nodes of upper layer
    int getParentLevel(Node* i_nodes, int& bgn, int& end, Node* o_nodes) {
        // std::cout << "bgn=" << bgn << ", end=" << end << std::endl;
        int size = end - bgn;
        std::sort(i_nodes, i_nodes + size, xNodeComp);
        int s = calcuSlice(size);
        int sn = s * NC;
        int slices_num = ceil(size, sn);
#pragma omp parallel for num_threads(32) schedule(dynamic)
        for (int i = 0; i < slices_num; i++) {
            if (i == slices_num - 1) {
                std::sort(i_nodes + i * sn, i_nodes + size, yNodeComp);
                parentNodes(i_nodes + i * sn, bgn + i * sn, size - i * sn, &o_nodes[i * s]);
            } else {
                std::sort(i_nodes + i * sn, i_nodes + (i + 1) * sn, yNodeComp);
                parentNodes(i_nodes + i * sn, bgn + i * sn, sn, &o_nodes[i * s]);
            }
        }

        int next_nodes_num = ceil(size, NC);
        bgn = end;
        end += next_nodes_num;
        // std::cout << "s=" << s << ", sn=" << sn << ", size=" << size << ", next_nodes_num=" << next_nodes_num
        //<< std::endl;
        return next_nodes_num;
    }

    // point in polygon, return polygon id + point id
    void contains(std::vector<std::pair<uint32_t, uint32_t> > polygon, std::vector<int>& results) {
        std::queue<Node> q_nodes;
        std::vector<Point> v_points;
        q_nodes.push(_index[_index_sz - 1]);
        Point* points = (Point*)_xys;
        int cnt = 0;
        while (!q_nodes.empty()) {
            cnt++;
            Node node = q_nodes.front();
            q_nodes.pop();
            if (node.level[16] == 0) {
                if (rectangle_polygon_intersect(polygon, node)) {
                    // std::cout << "INFO 0, level=" << node.level(7, 0) << ", true rectangle_polygon_intersect\n";
                    for (int i = 0; i < node.level(15, 8); i++) {
                        if (node.level(7, 0)) {
                            q_nodes.push(_index[node.addr + i]);
                        } else {
                            // printPoint(points[node.addr + i]);
                            v_points.push_back(points[node.addr + i]);
                        }
                    }
                } else if (point_in_rectangle(polygon[0], node)) {
                    // std::cout << "INFO 1, level=" << node.level(7, 0) << ", true rectangle contain polygon\n";
                    for (int i = 0; i < node.level(15, 8); i++) {
                        if (node.level(7, 0)) {
                            q_nodes.push(_index[node.addr + i]);
                        } else {
                            v_points.push_back(points[node.addr + i]);
                        }
                    }
                } else {
                    std::pair<uint32_t, uint32_t> vertex(node.xmin, node.ymin);
                    // std::cout << "polygon contain rectangle...\n";
                    if (point_in_polygon(vertex, polygon)) {
                        // std::cout << "INFO 2, level=" << node.level(7, 0) << ", true polygon contain rectangle\n";
                        for (int i = 0; i < node.level(15, 8); i++) {
                            if (node.level(7, 0)) {
                                Node next_node = _index[node.addr + i];
                                next_node.level[16] = 1;
                                q_nodes.push(next_node);
                            } else {
                                Point next_point = points[node.addr + i];
                                next_point.id[31] = 1;
                                // printPoint(next_point);
                                v_points.push_back(next_point);
                            }
                        }
                    }
                }
            } else {
                // std::cout << "INFO 3, level=" << node.level(7, 0) << ", true polygon contain rectangle\n";
                for (int i = 0; i < node.level(15, 8); i++) {
                    if (node.level(7, 0)) {
                        Node next_node = _index[node.addr + i];
                        next_node.level[16] = 1;
                        q_nodes.push(next_node);
                    } else {
                        Point next_point = points[node.addr + i];
                        next_point.id[31] = 1;
                        // printPoint(next_point);
                        v_points.push_back(next_point);
                    }
                }
            }
        }
        // std::cout << "Search Point Size: " << v_points.size() << std::endl;
        for (int i = 0; i < v_points.size(); i++) {
            if (v_points[i].id[31] == 0) {
                if (point_in_polygon(v_points[i], polygon)) {
                    // results.push_back(id);
                    results.push_back(v_points[i].id(30, 0));
                }
            } else {
                // results.push_back(id);
                results.push_back(v_points[i].id(30, 0));
            }
        }
        // results.push_back(std::pair<uint32_t, uint32_t>(cnt, v_points.size()));
    }

   public:
    STRTree() { std::cout << "Create STRTree.\n"; }

    // load file to buffer
    void loadFile(std::string file) {
        std::cout << "Load File\n";
        std::ifstream t;
        t.open(file);
        t.seekg(0, std::ios::end);
        uint64_t length = t.tellg();
        t.seekg(0, std::ios::beg);
        //_file_buf = new char[length];
        // t.read(_file_buf, length);
        int fd = open(file.c_str(), O_RDONLY);
        _file_buf = (char*)mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
        t.close();
    }

    // convert csv file to arrow format
    void convertArrow(const int blockSize, const int xCol, const int yCol) {
        auto table_buffer = std::make_shared<arrow::Buffer>(_file_buf);
        auto input = std::make_shared<arrow::io::BufferReader>(table_buffer);

        arrow::io::IOContext io_context = arrow::io::default_io_context();
        auto read_options = arrow::csv::ReadOptions::Defaults();
        read_options.block_size = blockSize; // Byte, max 1GB
        // read_options.use_threads = false;
        auto parse_options = arrow::csv::ParseOptions::Defaults();
        auto convert_options = arrow::csv::ConvertOptions::Defaults();

        auto maybe_reader =
            arrow::csv::TableReader::Make(io_context, input, read_options, parse_options, convert_options);
        std::shared_ptr<arrow::csv::TableReader> reader = *maybe_reader;
        auto maybe_table = reader->Read();
        table = maybe_table.ValueOrDie();
        _xs = table->column(xCol);
        _ys = table->column(yCol);
        std::cout << "row_num=" << table->num_rows() << ", col_num=" << table->num_columns()
                  << ", chunk_num=" << _xs->num_chunks() << ", len=" << _xs->length() << std::endl;
        // for (int i = 0; i < _xs->num_chunks(); i++) {
        //    std::cout << _xs->chunk(i)->length() << std::endl;
        //}
    }

    // quantify double (x, y) to uint32 (x, y), and filter outside points the zone
    void quanGeoData(Rectangle<double> zone) {
        _xys = aligned_alloc<uint32_t>(_xs->length() * 3);
        int id = 0, cnt = 0;
        for (int i = 0; i < _xs->num_chunks(); i++) {
            const uint8_t* x_arr = _xs->chunk(i)->data()->buffers[1]->data();
            const uint8_t* y_arr = _ys->chunk(i)->data()->buffers[1]->data();
            const double* x_arr64 = (const double*)x_arr;
            const double* y_arr64 = (const double*)y_arr;
            double dx = zone.xmax - zone.xmin, dy = zone.ymax - zone.ymin;
            for (int j = 0; j < _xs->chunk(i)->length(); j++) {
                if ((x_arr64[j] > zone.xmin) && (x_arr64[j] < zone.xmax) && (y_arr64[j] > zone.ymin) &&
                    (y_arr64[j] < zone.ymax)) {
                    _xys[cnt * 3] = (x_arr64[j] - zone.xmin) / dx * Q;
                    _xys[cnt * 3 + 1] = (y_arr64[j] - zone.ymin) / dy * Q;
                    _xys[cnt * 3 + 2] = id;
                    cnt++;
                }
                id++;
            }
        }
        _real_sz = cnt;
        std::cout << "total size=" << id << ", real size=" << cnt << std::endl;
    }

    void indexFPGA(double* zone) {
#ifdef _STRTree_Kernel_
        double* xs = aligned_alloc<double>(MSN);
        double* ys = aligned_alloc<double>(MSN);
        int sz = 0;
        for (int i = 0; i < _xs->num_chunks(); i++) {
            const uint8_t* x_arr = _xs->chunk(i)->data()->buffers[1]->data();
            const uint8_t* y_arr = _ys->chunk(i)->data()->buffers[1]->data();
            const double* x_arr64 = (const double*)x_arr;
            const double* y_arr64 = (const double*)y_arr;
            memcpy(xs + sz, x_arr64, _xs->chunk(i)->length() * sizeof(double));
            memcpy(ys + sz, y_arr64, _ys->chunk(i)->length() * sizeof(double));
            sz += _xs->chunk(i)->length();
        }
        PT* ext_point_buf0 = aligned_alloc<PT>(MSN * 3);

        NT* ext_node_buf0 = aligned_alloc<NT>(MSN);
        NT* ext_node_buf1 = aligned_alloc<NT>(MSN * 2);
        std::cout << "sz=" << sz << std::endl;

        // send task requests
        auto dram_buf_xs = strtree_acc::create_bufpool(vpp::input);
        auto dram_buf_ys = strtree_acc::create_bufpool(vpp::input);
        auto dram_buf_zone = strtree_acc::create_bufpool(vpp::input);
        auto dram_buf_point0 = strtree_acc::create_bufpool(vpp::bidirectional);
        auto dram_buf_node0 = strtree_acc::create_bufpool(vpp::bidirectional);
        auto dram_buf_node1 = strtree_acc::create_bufpool(vpp::input);

        strtree_acc::send_while([&]() -> bool {
            double* acc_xs = (double*)strtree_acc::alloc_buf(dram_buf_xs, sizeof(double) * MSN);
            double* acc_ys = (double*)strtree_acc::alloc_buf(dram_buf_ys, sizeof(double) * MSN);
            double* acc_zone = (double*)strtree_acc::alloc_buf(dram_buf_zone, sizeof(double) * 4);
            PT* acc_point0 = (PT*)strtree_acc::alloc_buf(dram_buf_point0, sizeof(PT) * MSN * 3);
            NT* acc_node0 = (NT*)strtree_acc::alloc_buf(dram_buf_node0, sizeof(NT) * MSN);
            NT* acc_node1 = (NT*)strtree_acc::alloc_buf(dram_buf_node1, sizeof(NT) * MSN * 2);

            memcpy(acc_xs, xs, sizeof(double) * MSN);
            memcpy(acc_ys, ys, sizeof(double) * MSN);
            memcpy(acc_zone, zone, sizeof(double) * 4);
            memcpy(acc_point0, ext_point_buf0, sizeof(PT) * MSN * 3);
            memcpy(acc_node0, ext_node_buf0, sizeof(NT) * MSN);
            memcpy(acc_node1, ext_node_buf1, sizeof(NT) * MSN * 2);

            strtree_acc::compute(sz, acc_xs, acc_ys, acc_zone, acc_point0, acc_point0, acc_node0, acc_node1, acc_node1);

            return 0;
        });

        strtree_acc::receive_all_in_order([&]() {
            PT* dst_point_buf0 = (PT*)strtree_acc::get_buf(dram_buf_point0);
            NT* dst_node_buf0 = (NT*)strtree_acc::get_buf(dram_buf_node0);
            memcpy(ext_point_buf0, dst_point_buf0, (sz + 2) * sizeof(PT));
            memcpy(ext_node_buf0, dst_node_buf0, sz * sizeof(NT));

        });
        struct timeval start_time, end_time;
        std::cout << "INFO: kernel start------" << std::endl;
        gettimeofday(&start_time, 0);
        strtree_acc::join();
        gettimeofday(&end_time, 0);
        std::cout << "INFO: kernel end------" << std::endl;
        std::cout << "INFO: Execution time " << tvdiff(&start_time, &end_time) << "ms" << std::endl;
        free(xs);
        free(ys);
        free(ext_node_buf1);
        _real_sz = ext_point_buf0[sz];
        _index_sz = ext_point_buf0[sz + 1];
        _xys = aligned_alloc<uint32_t>(_real_sz * 3);
        _index = aligned_alloc<Node>(_index_sz);
        // memcpy(_xys, ext_point_buf0, sz * sizeof(PT));
        // memcpy(_index, ext_node_buf0, sz * sizeof(NT));
        std::cout << "_real_sz=" << _real_sz << ", _index_sz=" << _index_sz << std::endl;
        for (int i = 0; i < _real_sz; i++) {
            _xys[i * 3] = ext_point_buf0[i](31, 0);
            _xys[i * 3 + 1] = ext_point_buf0[i](63, 32);
            _xys[i * 3 + 2] = ext_point_buf0[i](93, 64);
        }
        for (int i = 0; i < _index_sz; i++) {
            _index[i].level = ext_node_buf0[i](31, 0);
            _index[i].xmin = ext_node_buf0[i](63, 32);
            _index[i].xmax = ext_node_buf0[i](95, 64);
            _index[i].ymin = ext_node_buf0[i](127, 96);
            _index[i].ymax = ext_node_buf0[i](159, 128);
            _index[i].addr = ext_node_buf0[i](191, 160);
        }
        free(ext_point_buf0);
        free(ext_node_buf0);
#endif
    }

    // build index
    void index() {
        _index = aligned_alloc<Node>(_real_sz / NC * 2);
        Point* points = (Point*)_xys;
        struct timeval t1, t2;
        gettimeofday(&t1, 0);
        std::sort(points, points + _real_sz, xComp);
        gettimeofday(&t2, 0);
        std::cout << "data size=" << _real_sz << ", Sort Execution time " << tvdiff(&t1, &t2) / 1000.0 << " ms"
                  << std::endl;
        int s = calcuSlice(_real_sz);
        int sn = s * NC;
        // std::cout << "s=" << s << ", sn=" << sn << std::endl;

        // index point to lowest level
        int slices_num = ceil(_real_sz, sn);
#pragma omp parallel for num_threads(32) schedule(dynamic)
        for (int i = 0; i < slices_num; i++) {
            if (i == slices_num - 1) {
                std::sort(points + i * sn, points + _real_sz, yComp);
                parentNodes(points + i * sn, i * sn, _real_sz - i * sn, &_index[i * s]);
            } else {
                std::sort(points + i * sn, points + (i + 1) * sn, yComp);
                parentNodes(points + i * sn, i * sn, sn, &_index[i * s]);
            }
        }

        int bgn = 0;
        int end = ceil(_real_sz, NC);
        // up to index structure
        while (end - bgn != 1) {
            _level++;
            getParentLevel(_index + bgn, bgn, end, _index + end);
        }
        _index_sz = end;
        std::cout << "index size=" << _index_sz << std::endl;
    }

    // execute contains
    void containsAll(std::vector<std::vector<std::pair<uint32_t, uint32_t> > > polygons,
                     std::vector<std::vector<int> >& results) {
        Point* points = (Point*)_xys;
//#pragma omp parallel for schedule(dynamic)
#pragma omp parallel for num_threads(32) schedule(dynamic)
        for (int i = 0; i < polygons.size(); i++) {
            if (0) { // no index + search, for debug
                for (int j = 0; j < _real_sz; j++) {
                    if (point_in_polygon(points[j], polygons[i])) {
                        // results[i].push_back(i);
                        results[i].push_back(points[j].id);
                    }
                }
            } else // index + search
                contains(polygons[i], results[i]);
            // std::cout << "\ni=" << i << ", size=" << polygons[i].size() << ", result size=" << results[i].size()
            //          << std::endl;
        }
        free(_xys);
        free(_index);
    }
}; // strtree

// read polygon file to vector
void read_polygon(std::string file,
                  Rectangle<double> zone,
                  std::vector<std::vector<std::pair<uint32_t, uint32_t> > >& polygons) {
    std::ifstream in(file);
    std::string line;
    int polygon_num = 0;
    double dx = zone.xmax - zone.xmin, dy = zone.ymax - zone.ymin;
    while (std::getline(in, line)) {
        if (polygon_num + 1 > polygons.size()) {
            polygons.resize(polygons.size() + 1000);
        }
        std::stringstream ss(line);
        std::string tmp;
        int i = 0;
        uint32_t x, y;
        double x2, y2;
        while (std::getline(ss, tmp, ',')) {
            double d = std::stod(tmp);
            if (i % 2) {
                y2 = d;
                y = (d - zone.ymin) / dy * Q;
                polygons[polygon_num].push_back(std::pair<uint32_t, uint32_t>(x, y));
            } else {
                x2 = d;
                x = (d - zone.xmin) / dx * Q;
            }
            i++;
        }
        polygon_num++;
    }
    polygons.resize(polygon_num);
}

void copy2array(std::vector<std::vector<int> >& results, int* rangevec, int n) {
    int cnt = 0;
    for (int i = 0; i < results.size(); i++)
        for (int j = 0; j < results[i].size(); j++) {
            rangevec[cnt * 2] = i;
            rangevec[cnt * 2 + 1] = results[i][j];
            cnt++;
        }
}

int strtree_contains(int mode,
                     int x_col,
                     int y_col,
                     std::string point_file,
                     std::string polygon_file,
                     double zone[4],
                     std::vector<std::vector<int> >& results) {
    struct timeval t1, t2, t3, t4;
    std::cout << "Arrow Flow...\n";
    gettimeofday(&t1, 0);
    const int NodeCapacity = 16;
    STRTree<NodeCapacity> tree;
    tree.loadFile(point_file);

    Rectangle<double>* rect = (Rectangle<double>*)zone;

    std::vector<std::vector<std::pair<uint32_t, uint32_t> > > polygons(100);
    read_polygon(polygon_file, *rect, polygons);

    const int BlockSize = 1 << 26;
    tree.convertArrow(BlockSize, x_col, y_col);
    gettimeofday(&t2, 0);

    std::cout << "Index Flow...\n";
    if (mode == 0) {
        tree.quanGeoData(*rect);
        tree.index();
    } else {
        tree.indexFPGA(zone);
    }
    gettimeofday(&t3, 0);

    std::cout << "Search Flow...\n";
    results.resize(polygons.size());
    tree.containsAll(polygons, results);

    int total_cnt = 0;
    for (int i = 0; i < results.size(); i++) total_cnt += results[i].size();
    gettimeofday(&t4, 0);

    std::cout << "Convert Arrow Execution time " << tvdiff(&t1, &t2) / 1000.0 << " ms" << std::endl;
    std::cout << "Index Execution time " << tvdiff(&t2, &t3) / 1000.0 << " ms" << std::endl;
    std::cout << "Search Execution time " << tvdiff(&t3, &t4) / 1000.0 << " ms" << std::endl;
    std::cout << "Total Execution time " << tvdiff(&t1, &t4) / 1000.0 << " ms" << std::endl;
    return total_cnt;
}

} // geospatial
} // data_analytics
} // xf
