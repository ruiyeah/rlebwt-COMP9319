#include <iostream>
#include <string>
#include <cstdio>
#include <unistd.h>
#include <vector>
#include <map>
#include "sys/types.h"
#include "sys/stat.h"


using namespace std;
// the max size of buffers
#define MAXSIZE 10240
#define BITMAX MAXSIZE*8
int i, j, k;


std::string bbFN, bFN, sFN;
char s_buffer[MAXSIZE];
char b_buffer[MAXSIZE];
char bb_buffer[MAXSIZE];
char fs[MAXSIZE];
vector<bool> b_arr, bb_arr;
vector<unsigned int> select_b,select_bb;
map<char,unsigned int> cs_table;
map<unsigned int,unsigned int> rank_b,rank_bb;
FILE *sp, *bp, *bbp;
int flag = 0, count_of_char = 0, last = 0;
int baseline = 0, length_prev = 0, length_next = 0, status = 0;


void printBitArray(vector<bool> &arr) {
	for (i = 0; i < arr.size(); i++)//size()容器中实际数据个数
	{
		cout << arr[i];
		if ((i + 1) % 8 == 0) {
			cout << " ";
		}
	}
	cout << endl;
}

void writeBitToBitArray(vector<bool> &arr, int l) {
	arr.push_back(true);
	while (l > 0) {
		arr.push_back(false);
		l--;
	}
}

void getBitArray(vector<bool> &arr, char *str, int &count, int &last) {
	for (i = 0; i < strlen(str); i++) {
		for (j = 7; j > -1; j--) {
			arr.push_back(str[i] & (1 << j));
			if (str[i] & (1 << j)) {
				count++;
				last = 8 * i + (7 - j);
			}
		}
	}
}

void convertBitsToString(vector<bool> &arr, char (&str)[MAXSIZE]) {
	//printBitArray(arr);
	for (i = 0; i < arr.size() / 8; i++) {
		str[i] = 0;
		for (j = 0; j < 8; j++) {
			//cout<<(arr[8*i+j]<<(7-j))<<" ";
			str[i] |= arr[8 * i + j] << (7 - j);
		}
	}
	//cout<<endl;
}

void sortFS(char (&fs)[MAXSIZE]) {
	int fs_size = (int) strlen(fs);
	int min;
	char tmp;
	for (i = 0; i < fs_size; i++) {
		min = i;
		for (j = i + 1; j < fs_size; j++) {
			if (fs[j] < fs[min]) {
				min = j;
			}
		}
		tmp = fs[i];
		fs[i] = fs[min];
		fs[min] = tmp;
	}
}

void sortFSAndBB(char (&fs)[MAXSIZE], vector<bool> &bb) {
	int fs_size = (int) strlen(fs);
	int b_size = (int) bb.size();
	char tmp;
	baseline = 0, length_prev = 0, length_next = 0, status = 0;
	for (i = 0; i < fs_size - 1; i++) {
		baseline = 0;
		for (j = 0; j < fs_size - 1 - i; j++) {
			length_next = 0;
			length_prev = 0;
			for (k = baseline; k < b_size; k++) {
				if (bb[k]) {
					status++;
				}
				if (status == 1) {
					length_prev++;
				} else if (status == 2) {
					length_next++;
				} else if (status == 3) {
					break;
				}
			}
			status = 0;
			if (fs[j] > fs[j + 1]) {
				//cout<<baseline<<endl;
				//cout<<fs[j]<<length_prev<<","<<fs[j+1]<<length_next<<endl;
				tmp = fs[j + 1];
				fs[j + 1] = fs[j];
				fs[j] = tmp;
				if (length_next != length_prev) {
					bb[baseline + length_next] = true;
					bb[baseline + length_prev] = false;
				}
				/*
				for(int a=0;a<b_size;a++){
					cout<<bb[a];
					if((a+1)%8==0){
						cout<<" ";
					}
				}
				//cout<<endl;
				 */

				baseline += length_next;

			} else {
				baseline += length_prev;
			};

			//cout<<baseline<<endl;
		}
		//printBitArray(bb);
	}
}

void readSB(string &fileName) {
	bbFN = fileName + ".bb";
	sFN = fileName + ".s";
	bFN = fileName + ".b";
	sp = fopen(sFN.c_str(), "r");
	bp = fopen(bFN.c_str(), "r");
	bbp = fopen(bbFN.c_str(),"r");
	bool bbp_exist = true;
	if (!bbp) {
		bbp_exist = false;
	}
	if (!sp || !bp) {
		cout << fileName + ".s/.b not exists!" << endl;
	}
	while (!feof(sp) || !feof(bp)) {
		if (!feof(sp)) {
			fgets(s_buffer, MAXSIZE, sp);
		}
		if (!feof(bp)) {
			count_of_char = 0;
			fgets(b_buffer, MAXSIZE, bp);
			getBitArray(b_arr, b_buffer, count_of_char, last);
			while (!flag) {
				strcpy(fs, s_buffer);
				if (!bbp_exist) {
					bb_arr.assign(b_arr.begin(), b_arr.end());
					sortFSAndBB(fs, bb_arr);
					convertBitsToString(bb_arr, bb_buffer);
					printBitArray(bb_arr);
					if (!(bbp = fopen(bbFN.c_str(), "r"))) {
						fclose(bbp);
						bbp = fopen(bbFN.c_str(), "w");
						fwrite(bb_buffer, strlen(bb_buffer), 1, bbp);
					} else {
						bbp = fopen(bbFN.c_str(), "w");
						fwrite(bb_buffer, strlen(bb_buffer), 1, bbp);
					}
				} else {
					fgets(bb_buffer, MAXSIZE, bbp);
					getBitArray(bb_arr, bb_buffer, count_of_char, last);
					sortFS(fs);
					printBitArray(bb_arr);
				}
				break;
			}
		}
	}
}

void generateIndex(){
	//get cs_table
	char prev_char;
	baseline=-1;
	cout<<fs<<endl;
	unsigned int count_b=0,count_bb=0;
	for(i=0;i<strlen(fs);i++){
		while(true){
			baseline++;
			if(b_arr[baseline]){
				count_b++;
				select_b.push_back((unsigned int)baseline);
				rank_b[baseline+1]=count_b;
			}
			if(bb_arr[baseline]){
				count_bb++;
				select_bb.push_back((unsigned int)baseline);
				rank_bb[baseline+1]=count_bb;
				break;
			}
		}
		if(prev_char!=fs[i]){
			cs_table[fs[i]]=baseline;
			prev_char=fs[i];
		}
	}

	for(i=0;i<select_bb.size();i++){
		cout<<i<<","<<select_b[i]<<endl;
	}


	map<unsigned int,unsigned int>::iterator iter;
	for(iter = rank_bb.begin(); iter != rank_bb.end(); iter++) {
		cout << iter->first << " : " << iter->second << endl;
	}

}

void searchForTimes(string target){

}


int main(int argc, char *argv[]) {
	if (argc < 4) {

	}

	std::string fileName = argv[2];
	std::string mode = argv[1];
	std::string target = argv[4];
	readSB(fileName);

	//after read , Assume that all S B and BB is good.
	generateIndex();
	if(mode=="-m"){
		searchForTimes(target);
	}



	if (access(argv[3], 00) < 0) {
		mkdir(argv[3], 0777);
	}


	return 0;
}