### Error:
Missed the template parameter 'MAX_DOWN_SCALE' in the Resize function API.

## Correct API:

template < int INTERPOLATION_TYPE, int TYPE, int SRC_ROWS, int SRC_COLS, int DST_ROWS, int DST_COLS, int NPC, int MAX_DOWN_SCALE >   
void resize (xf::Mat<TYPE, SRC_ROWS, SRC_COLS, NPC> &_src, xf::Mat<TYPE, DST_ROWS, DST_COLS, NPC> & _dst);

##### Description of missed template parameter:

MAX_DOWN_SCALE	- Set to 2 for all 1-pixel modes, and for upscale in x direction.
When down scaling in x direction in 8-pixel mode, please set this parameter to the next highest integer value of the down scale factor i.e., if downscaling from 1920 columns to 1280 columns, set to 2. For 1920 to 640, set to 3.

