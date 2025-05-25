
//#define DIM 16

#define COLID(x) (x)

const unsigned int DIM = 32;
const unsigned int M = DIM;
const unsigned int N = DIM;
const unsigned int ROW = M;
const unsigned int COL = N;
const unsigned int BlkNum = 4;
const unsigned int CoreNum = DIM / BlkNum;

std::string fin = "data/in_" + std::to_string(DIM) + "_" + std::to_string(BlkNum) + ".txt";
std::string fgld = "data/gld_" + std::to_string(DIM) + "_" + std::to_string(BlkNum) + ".txt";
std::string fout = "data/out_" + std::to_string(DIM) + "_" + std::to_string(BlkNum) + ".txt";
