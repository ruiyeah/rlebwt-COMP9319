//
// Created by rui on 2019-07-28.
//

#ifndef ASS2CPP_PUBLIC_H
#define ASS2CPP_PUBLIC_H

using namespace std;

//MAXSIZE should larger than SECTIONSIZE

#define MAX_SIZE 40960
#define BITMAX MAXSIZE*8
#define CHARSCALE 128

#define CHAR_SECTION_SIZE 40960

//section size should >= 8
#define SECTION_SIZE 1024

//in char i.e. 8 bits
//dont know why...this size can't be 102400 =_=...
#define BB_BUFFER_SIZE 409600

void printBitArray(std::vector<bool> &arr);
void writeBitToBitArray(vector<bool> &arr, int l);
void printSelectBitArray(vector<bool> &select_b);
void printRankBMap(map<unsigned int,unsigned int> &rank_b);

#endif //ASS2CPP_PUBLIC_H
