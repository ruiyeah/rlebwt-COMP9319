#include <iostream>
#include <string>
#include <cstdio>
#include <unistd.h>
#include <vector>
#include <map>
#include "sys/types.h"
#include "sys/stat.h"

#include "utils.h"

using namespace std;
// vars for loop.
unsigned int i, j, k, a, s, d, q, w, e, sbb;
unsigned int outer = 0, inner = 0;


unsigned int MAXSIZE=MAX_SIZE;
unsigned int SECTIONSIZE=SECTION_SIZE;
unsigned int CHARSECTIONSIZE=SECTION_SIZE;
unsigned int BIT_SECTION_SIZE_OF_CHAR=SECTIONSIZE/8;

//buffers
std::string bbFN, bFN, sFN;

char *s_buffer;
char *b_buffer;
char *bb_buffer;
char *bb_buffer_for_generate;

//for s
unsigned int cs_table[CHARSCALE], lens_table[CHARSCALE];
vector<unsigned int> select_s;
unsigned int s_section_count = 0, current_s_buffer_section = 0;
unsigned int current_s_buffer_size = 0;

//for b
vector<unsigned int> rank_b, select_b;
unsigned int rank_b_section_count = 0, select_b_section_count = 0, char_b_count = 0;
unsigned int count_of_b = 0, suffix_b1_start;
unsigned int current_b_buffer_size = 0, current_b_buffer_section = 0;

//for bb
unsigned int current_bb_buffer_size = 0, current_bb_buffer_section = 0, select_bb_section_count = 0;
unsigned int count_bb_1 = 0;
vector<unsigned int> select_bb;

//for search
map<unsigned int, bool> prevPosTable;
vector<int> mappingIndex;

FILE *sp, *bp, *bbp;
unsigned int count_of_s = 0;

bool isNum(string str) {
	for (d = 0; d < str.length(); d++) {
		if (!isdigit(str[d])) {
			return false;
		}
	}
	return true;
}

void init() {
	for (i = 0; i < CHARSCALE; i++) {
		//cout<<(char)i;
		cs_table[i] = 0;
		lens_table[i] = 0;
		select_s.push_back(0);
	}
	rank_b.push_back(0);
	select_b.push_back(0);
	select_bb.push_back(0);
}


void printLengthTableOfS() {
	for (int i = 0; i < CHARSCALE; i++) {
		if (lens_table[i] > 0) {
			cout << char(i) << "," << lens_table[i] << endl;
		}
	}
}


void initBB() {
	bbp = fopen(bbFN.c_str(), "w+");
	//cout<<bbFN<<endl;
	//cout<<rank_b_section_count<<endl;
	//cout<<char_b_count<<endl;
	if(!bbp){
		cout<<"no bb"<<endl;
	}
	for (i = 0; i < rank_b_section_count; i++) {
		//cout<<i<<endl;

		char tmp[SECTIONSIZE / 8];
		for (j = 0; j < SECTIONSIZE; j += 8) {
			tmp[j / 8] = -1;
		}
		fwrite(tmp, SECTIONSIZE / 8, 1, bbp);
	}

	if (char_b_count > (SECTIONSIZE * rank_b_section_count / 8)) {
		unsigned int left_count = (unsigned int) char_b_count % (SECTIONSIZE / 8);
		char tmp[left_count];
		for (j = 0; j < left_count; j++) {
			tmp[j] = -1;
		}
		fwrite(tmp, left_count, 1, bbp);
	}
}

void readSBySection(unsigned int &target_section) {
	//target section not in current buffer.
	if ((target_section - current_s_buffer_section) * CHARSECTIONSIZE / MAXSIZE > 0 ||
		current_s_buffer_section > target_section) {
		fseek(sp, target_section * CHARSECTIONSIZE, 0);
		current_s_buffer_size = (unsigned int) fread(s_buffer, 1, MAXSIZE, sp);
		current_s_buffer_section = target_section;
	}
}

void readBBySection(unsigned int &target_section) {
	//target section not in current buffer.
	if ((target_section - current_b_buffer_section) * SECTIONSIZE / MAXSIZE > 0 ||
		current_b_buffer_section > target_section) {
		fseek(bp, target_section * BIT_SECTION_SIZE_OF_CHAR, 0);
		current_b_buffer_size = (unsigned int) fread(b_buffer, 1, MAXSIZE, bp);
		current_b_buffer_section = target_section;
	}
}

void readBBBySection(unsigned int &target_section) {
	//target section not in current buffer.
	if ((target_section - current_bb_buffer_section) * SECTIONSIZE / MAXSIZE > 0 ||
		current_bb_buffer_section > target_section) {
		fseek(bbp, target_section * BIT_SECTION_SIZE_OF_CHAR, 0);
		current_bb_buffer_size = (unsigned int) fread(bb_buffer, 1, MAXSIZE, bbp);
		current_bb_buffer_section = target_section;
	}
}


//for selectS
unsigned int result_select_s = 0;
int gap_select_s = 0;
unsigned int prev_select_s = 0;
unsigned int lower_bound_select_s = 0;

unsigned lower_s=0,upper_s=0,mid_s=0;
unsigned int bSearchOfSelectS(unsigned int &target_num, unsigned int &target_char_int){
	lower_s=0;
	upper_s=s_section_count;
	if(target_num>select_s[upper_s * CHARSCALE + target_char_int]){
		return upper_s+1;
	}
	while(true){
		mid_s=(lower_s+upper_s)/2;
		if(target_num>select_s[mid_s * CHARSCALE + target_char_int]){
			lower_s=mid_s;
		}else if(target_num<=select_s[mid_s * CHARSCALE + target_char_int]){
			upper_s=mid_s;
		}
		if((upper_s-lower_s)/2==0){
			return upper_s;
		}
	}
}

unsigned int selectS(unsigned int target_num, unsigned int target_char_int) {
	char target_char = (char) target_char_int;
	target_num += 1;
	k=bSearchOfSelectS(target_num,target_char_int);
	//while k=0,all rank_s[char] is 0,hence k start at 1.
	lower_bound_select_s = k - 1;
	// now k is the section that occ[x] > target;
	result_select_s = (unsigned int) CHARSECTIONSIZE * lower_bound_select_s;
	prev_select_s = (unsigned int) select_s[lower_bound_select_s * CHARSCALE + target_char_int];
	//if the target section in the current buffer?
	gap_select_s = (k - 1) - current_s_buffer_section;
	readSBySection(lower_bound_select_s);

	//yes.
	if (gap_select_s >= 0 && ((unsigned int)gap_select_s < (unsigned int)(MAXSIZE / CHARSECTIONSIZE))) {
		for (a = gap_select_s * (unsigned int)CHARSECTIONSIZE; a < (k - current_s_buffer_section) * CHARSECTIONSIZE; a++) {
			if (s_buffer[a] == target_char) {
				prev_select_s++;
				if (prev_select_s == target_num) {
					result_select_s = a + current_s_buffer_section * CHARSECTIONSIZE;
					return result_select_s;
				}
			}
		}
	} else {
		//no,fetch the new s_buffer.
		readSBySection(lower_bound_select_s);
		for (a = 0; a < CHARSECTIONSIZE; a++) {
			if (s_buffer[a] == target_char) {
				prev_select_s++;
				if (prev_select_s == target_num) {
					result_select_s += a;
					return result_select_s;
				}
			}
		}
	}
	return count_of_s;
}

//rankS : target num should larger than 0;now it calculate the char at target_num itself.
unsigned int lower_bound_section_rank_s = 0;
unsigned int prev_sum_of_char = 0;

unsigned int rankS(unsigned int target_num, char target_char) {
	if (target_num < 1) {
		return 0;
	}
	target_num -= 1;
	int target_char_int = (int) target_char;
	if (target_num > count_of_s) {
		return lens_table[target_char_int];
	}
	lower_bound_section_rank_s = target_num / CHARSECTIONSIZE;
	prev_sum_of_char = select_s[lower_bound_section_rank_s * CHARSCALE + target_char_int];
	readSBySection(lower_bound_section_rank_s);

	for (e = (lower_bound_section_rank_s - current_s_buffer_section) * CHARSECTIONSIZE;
		 e <= target_num - current_s_buffer_section * CHARSECTIONSIZE; e++) {
		if (s_buffer[e] == target_char) {
			prev_sum_of_char++;
		}
	}
	return prev_sum_of_char;
}


//rankB
unsigned int lower_bound_section_rank_b = 0;
unsigned int offset_section_rank_b = 0;

unsigned int rankB(unsigned int target_num) {
	unsigned int prev_sum_rank_b = 0;
	//cout<<"suffix<<"<<suffix_b1_start<<endl;
	if (target_num > suffix_b1_start && suffix_b1_start > 0) {
		return count_of_b;
	}
	lower_bound_section_rank_b = target_num / SECTIONSIZE;
	prev_sum_rank_b = rank_b[lower_bound_section_rank_b];

	//cout<<endl<<"lower_bound_section_rank_b : "<<lower_bound_section_rank_b<<endl;
	//cout<<"prev_sum_rank_b : "<<prev_sum_rank_b<<endl;

	readBBySection(lower_bound_section_rank_b);
	offset_section_rank_b = lower_bound_section_rank_b - current_b_buffer_section;
	//cout<<"offset_section_rank_b : "<<offset_section_rank_b<<endl;
	//cout<<"upper : "<<(target_num/8 - offset_section_rank_b * BIT_SECTION_SIZE_OF_CHAR)<<endl;

	for (outer = offset_section_rank_b * BIT_SECTION_SIZE_OF_CHAR;
		 outer < (target_num / 8 - current_b_buffer_section * BIT_SECTION_SIZE_OF_CHAR); outer++) {
		//cout<<"outer: "<<outer<<endl;
		for (inner = 0; inner < 8; inner++) {
			if (b_buffer[outer] & (128 >> inner)) {
				prev_sum_rank_b++;
			}
		}
	}
	if (target_num % 8 > 0) {
		for (inner = 0; inner < target_num % 8; inner++) {
			if (b_buffer[outer] & (128 >> inner)) {
				prev_sum_rank_b++;
			}
		}
	}

	return prev_sum_rank_b;
}

//get the following zero of a bit 1 inside B
unsigned int char_loc = 0;
unsigned int section_of_loc = 0;

unsigned int getZeros(unsigned int location) {
	unsigned int result = 0;
	while (true) {
		location++;
		char_loc = location / 8;
		//if this char in the current buffer?
		section_of_loc = char_loc / BIT_SECTION_SIZE_OF_CHAR;
		//not in buffer
		if (current_b_buffer_section > section_of_loc ||
			section_of_loc - current_b_buffer_section >= MAXSIZE / SECTIONSIZE) {
			readBBySection(section_of_loc);
		}
		if (!(b_buffer[location / 8 - current_b_buffer_section * BIT_SECTION_SIZE_OF_CHAR] & (128 >> (location % 8)))) {
			result++;
		} else {
			return result;
		}
	}
}

//write the zeros value into the bb_buffer
void writeZerosIntoBB(unsigned int &baseline_bb, unsigned int zeros) {
	while (zeros > 0) {
		//current location not in buffer: write the current content to file,
		//and then read the next section.
		if ((baseline_bb / 8) >= current_bb_buffer_size + BB_BUFFER_SIZE * current_bb_buffer_section) {
			fseek(bbp, -BB_BUFFER_SIZE, 1);
			fwrite(bb_buffer_for_generate, 1, current_bb_buffer_size, bbp);
			current_bb_buffer_size = (unsigned int) fread(bb_buffer_for_generate, 1, BB_BUFFER_SIZE, bbp);
			current_bb_buffer_section++;
		}
		char_loc = baseline_bb / 8 - current_bb_buffer_section * BB_BUFFER_SIZE;
		bb_buffer_for_generate[char_loc] &= (255 - (128 >> (baseline_bb % 8)));
		baseline_bb++;
		zeros--;
	}
}


int gap_select_b = 0;
unsigned int lower_bound_select_b = 0;
unsigned int result_select_b = 0, prev_select_b = 0;
unsigned int start_outer = 0;

unsigned lower_b=0,upper_b=0,mid_b=0;
unsigned bSearchOfSelectB(unsigned int &target_num){
	lower_b=0;
	upper_b=rank_b_section_count;
	if(target_num>rank_b[upper_b]){
		return upper_b+1;
	}
	while(true){
		mid_b=(lower_b+upper_b)/2;
		if(target_num>rank_b[mid_b]){
			lower_b=mid_b;
		}else if(target_num<=rank_b[mid_b]){
			upper_b=mid_b;
		}
		if((upper_b-lower_b)/2==0){
			return upper_b;
		}
	}
}

unsigned int selectB(unsigned int target_num) {
	target_num += 1;
	w=bSearchOfSelectB(target_num);
	lower_bound_select_b = w - 1;
	result_select_b = rank_b[lower_bound_select_b];
	prev_select_b = rank_b[lower_bound_select_b];
	gap_select_b = lower_bound_select_b - current_b_buffer_section;
	//in buffer
	if (gap_select_b >= 0 && ((unsigned int)gap_select_b < (8 * MAXSIZE / SECTIONSIZE))) {
		for (outer = (unsigned int)gap_select_b * BIT_SECTION_SIZE_OF_CHAR;
			 outer <= (w - current_b_buffer_section) * BIT_SECTION_SIZE_OF_CHAR; outer++) {
			//every char
			for (inner = 0; inner < 8; inner++) {
				//every bit in each char
				if (b_buffer[outer] & (128 >> inner)) {
					prev_select_b++;
					if (prev_select_b == target_num) {
						result_select_b = inner + 8 * (outer + current_b_buffer_section * BIT_SECTION_SIZE_OF_CHAR);
						return result_select_b;
					}
				}
			}
		}
	} else {
		//not in buffer
		readBBySection(lower_bound_select_b);
		for (outer = 0; outer < BIT_SECTION_SIZE_OF_CHAR; outer++) {
			//every char
			for (inner = 0; inner < 8; inner++) {
				//every bit in each char
				if (b_buffer[outer] & (128 >> inner)) {
					prev_select_b++;
					if (prev_select_b == target_num) {
						result_select_b = inner + 8 * (outer + current_b_buffer_section * BIT_SECTION_SIZE_OF_CHAR);
						return result_select_b;
					}
				}
			}
		}
	}
	return 0;
}

//select bb
unsigned int lower_bound_section_select_bb = 0;
unsigned int pos_select_bb = 0, prev_sum_select_bb = 0;
unsigned int offset_section_select_bb = 0, start_pos = 0;

unsigned int selectBB(unsigned int target_num) {
	target_num += 1;
	if (target_num % SECTIONSIZE == 0) {
		return select_bb[target_num / SECTIONSIZE];
	}
	prev_sum_select_bb = target_num - (target_num % SECTIONSIZE);
	pos_select_bb = select_bb[target_num / SECTIONSIZE];
	//result is a position.
	lower_bound_section_select_bb = pos_select_bb / SECTIONSIZE;
	start_pos = pos_select_bb % 8;
	readBBBySection(lower_bound_section_select_bb);
	offset_section_select_bb = lower_bound_section_select_bb - current_bb_buffer_section;
	if (prev_sum_select_bb > 0) {
		prev_sum_select_bb--;
	}

	start_outer = (pos_select_bb - current_bb_buffer_section * SECTIONSIZE) / 8;
	for (inner = 0; inner < start_pos; inner++) {
		pos_select_bb--;
		if (bb_buffer[start_outer] & (128 >> inner)) {
			prev_sum_select_bb--;
		}
	}
	while (true) {
		for (outer = start_outer;
			 outer < current_bb_buffer_size; outer++) {
			//cout<<"outer:"<<outer<<", ";
			for (inner = 0; inner < 8; inner++) {
				if (bb_buffer[outer] & (128 >> inner)) {
					prev_sum_select_bb++;
					if (prev_sum_select_bb == target_num) {
						//cout<<"inner:"<<inner<<", ";
						pos_select_bb += inner;
						return pos_select_bb;
					}
				}
			}
			pos_select_bb += 8;
		}
		lower_bound_section_select_bb += (MAXSIZE / SECTIONSIZE);
		readBBBySection(lower_bound_section_select_bb);
		start_outer = 0;
		if (feof(bbp)) {
			break;
		}
	}
	return pos_select_bb;
}

//generate the bb file.
void generateBB() {
	initBB();
	rewind(sp);
	rewind(bp);
	if(bbp){
		fclose(bbp);
	}
	bbp = fopen(bbFN.c_str(), "r+");
	bb_buffer_for_generate = new char[BB_BUFFER_SIZE];
	current_s_buffer_size = (unsigned int) fread(s_buffer, 1, MAXSIZE, sp);
	current_b_buffer_size = (unsigned int) fread(b_buffer, 1, MAXSIZE, bp);
	current_bb_buffer_size = (unsigned int) fread(bb_buffer_for_generate, 1, BB_BUFFER_SIZE, bbp);
	current_bb_buffer_section = 0;
	//cout<<current_s_buffer_size<<endl;
	//cout<<current_b_buffer_size<<endl;
	//cout<<current_bb_buffer_size<<endl;
	//cout<<"generate the bb file."<<endl;
	unsigned int zeros = 0;
	unsigned int total_zeros = 0;
	unsigned int baseline_bb = 0;
	/*
	for(i=0;i<=count_of_s;i++){
		cout<<i+1<<", "<<select_b[i/SECTIONSIZE]<<", "<<selectB(i)<<endl;
	}*/

	for (i = 0; i < CHARSCALE; i++) {
		if (lens_table[i] < 1) {
			continue;
		}
		j = 0;
		while (j < lens_table[i]) {


			zeros = getZeros(selectB(selectS(j, i)));
			baseline_bb++;
			if (zeros > 0) {
				total_zeros += zeros;
				writeZerosIntoBB(baseline_bb, zeros);
			}
			//select_bb : x: the section num of each section 1's y: the position.
			if ((baseline_bb + 1 - total_zeros) % SECTIONSIZE == 0) {
				select_bb.push_back(baseline_bb);
				select_bb_section_count++;
			}
			j++;
		}
	}
	count_bb_1 = baseline_bb - total_zeros;
	//final write
	fseek(bbp, current_bb_buffer_section * BB_BUFFER_SIZE, 0);
	fwrite(bb_buffer_for_generate, 1, current_bb_buffer_size, bbp);
	rewind(bbp);
	delete[]bb_buffer_for_generate;
	current_bb_buffer_size = (unsigned int) fread(bb_buffer, 1, MAXSIZE, bbp);
	current_bb_buffer_section = 0;

	/*
	for(i=1;i<=baseline_bb-total_zeros;i++){
		cout<<i<<", "<<select_bb[i/8]<<", "<<selectBB(i)<<endl;
	}
	 */


}

//if bb exists, get the select table;
void constructBBIndex() {
	//cout<<"construct index"<<endl;
	fclose(bbp);
	bbp = fopen(bbFN.c_str(), "r");
	rewind(bbp);
	unsigned int bb_count = 0;
	unsigned int prev_bb_char_count = 0;
	while (!feof(bbp)) {
		current_bb_buffer_size = (unsigned int) fread(bb_buffer, 1, MAXSIZE, bbp);
		//cout<<current_bb_buffer_size<<endl;
		//cout<<i<<endl;
		for (i = 0; i < current_bb_buffer_size; i++) {
			//cout<<bb_count<<endl;
			for (j = 0; j < 8; j++) {
				if (bb_buffer[i] & (128 >> j)) {
					bb_count++;
					//select_bb : x: the section num of each section 1's y: the position.
					if (bb_count % SECTIONSIZE == 0) {
						//cout<<(prev_bb_char_count+i)*8+j<<endl;
						select_bb.push_back((prev_bb_char_count + i) * 8 + j);
						select_bb_section_count++;
					}
				}
			}
		}
		//cout<<bb_count<<endl;
		prev_bb_char_count += current_bb_buffer_size;
		current_bb_buffer_section = prev_bb_char_count / (BIT_SECTION_SIZE_OF_CHAR);
	}

	//cout<<bb_count<<endl;
	/*
	for (i = 0; i <= count_of_s; i++) {
		//cout<<i+1<<", "<<select_bb[(i+1)/SECTIONSIZE]<<", "<<selectBB(i)+1<<endl;
	}
	 */


}

//convert 0 base pos to 1 base index.
unsigned int selectBBForSearch(unsigned int target_num) {
	//cout<<"selectBBForSearch: "<<selectBB(target_num)+1<<endl;
	return selectBB(target_num) + 1;
}


//
unsigned int occS(char c, unsigned int num) {
	return rankS(rankB(num), c);
}

unsigned int lb = 0;

char getCharAtS(unsigned int target_num) {
	lb = target_num / CHARSECTIONSIZE;
	readSBySection(lb);
	return s_buffer[target_num - (current_s_buffer_section) * CHARSECTIONSIZE];
}


void backwardSearch(string &target, int &f_result, int &l_result) {
	int length = (int) target.length();
	int loc = length - 1;
	char current_char = target[loc];
	int current_char_int = (int) current_char;
	int fst = selectBBForSearch(cs_table[current_char_int]);
	int lst = selectBBForSearch(cs_table[current_char_int] + lens_table[current_char_int]) - 1;
	int first_i = 0;
	int first_p = 0, last_p = 0;
	//rank B : input is pos+1 (i*8+j +1)
	//cout<<"test"<<endl;
	//cout<<rankB((unsigned)0)<<endl;
	//cout<<rankB((unsigned)1)<<endl;
	//cout<<rankB((unsigned)2)<<endl;
	//cout<<rankB((unsigned)11)<<endl;
	//cout<<rankS(212,'n')<<endl;
	//cout<<rankS(1,'[')<<endl;
	//cout<<target<<endl;
	//cout<<target[loc]<<endl;
	//cout<<cs_table[current_char_int]+lens_table[current_char_int]<<endl;
	//cout<<lst<<endl;
	//cout<<selectBBForSearch(51925525)<<endl;
	//cout<<occS('a',3)<<endl;
	//unsigned lst=selectBB(cs_table[current_char_int]+rankS(rankB(loc),current_char));
	while ((fst <= lst) && loc >= 1) {
		//cout<<"fst: "<<fst<<"  lst: "<<lst<<endl;
		current_char = target[loc - 1];
		current_char_int = (int) current_char;
		first_i = fst - 1;
		//公式中的c[x] + 1 与 selectBB参数所需的-1 抵消.
		first_p = cs_table[current_char_int] + 1 + occS(current_char, fst - 1) - 1;
		last_p = cs_table[current_char_int] + 1 + occS(current_char, lst) - 1;
		/*
		cout<<"current_char: "<<current_char<<", "<<"rankB: fst-1 "<<first_i<<", "<<rankB((unsigned)first_i);
		cout<<"  rankS: "<<occS(current_char,(unsigned)first_i)<<endl;
		cout<<"current_char: "<<current_char<<", "<<"rankB: lst "<<lst<<", "<<rankB((unsigned)lst);
		cout<<"  rankS: "<<occS(current_char,(unsigned)lst)<<endl;
		cout<<"first pointer: "<<first_p <<endl;
		cout<<"last pointer: "<<last_p <<endl;
		*/



		if (getCharAtS(rankB(first_i) - 1) == current_char) {
			//cout<<"same fst"<<endl;
			fst = selectBBForSearch(cs_table[current_char_int] + occS(current_char, first_i)) + (first_i) -
				  selectB(rankB(first_i));
		} else {
			//cout<<first_p<<endl;
			fst = selectBBForSearch(first_p);
		}
		if (getCharAtS(rankB(lst) - 1) == current_char) {
			//cout<<"same lst"<<endl;
			lst = selectBBForSearch(cs_table[current_char_int] + occS(current_char, lst)) + lst - selectB(rankB(lst)) -
				  1;
		} else {
			//cout<<selectBBForSearch(last_p)<<endl;
			lst = selectBBForSearch(last_p) - 1;
		}
		loc--;
	}
	//cout<<"fst: "<<fst<<"  lst"<<lst<<endl;
	//cout<<"result:"<<lst-fst+1<<endl;
	f_result = (unsigned) fst;
	l_result = (unsigned) lst;
}

int f_result = 0, l_result = 0;
unsigned int rank_b_val=0;


//b->bb  L->F
unsigned int backwordDecode(unsigned int &target_num,char &c,unsigned int &rank_b_val_bd) {
	//cout<<"c: "<<c<<" rankB(target_num): "<<rankB(target_num)<<" selectB(rankB(target_num)): "<<selectB(rankB(target_num)-1)-1<<endl;
	//cout<<"cs_table: "<<cs_table[(int)c]<<" occS: "<<occS(c,target_num)<<" selectBB: "<<selectBBForSearch(cs_table[(int)c]+occS(c,target_num)-1)<<endl;

	return selectBBForSearch(cs_table[(int) c] + occS(c, target_num) - 1) + target_num - selectB(rank_b_val_bd - 1) - 1;

}

void searchForTimes(string target) {
	backwardSearch(target, f_result, l_result);
	if (l_result - f_result + 1 > 0) {
		int result=l_result - f_result + 1;
		if(!isNum(target)){
			cout << l_result - f_result + 1 << endl;
		}else{
			unsigned int current_p = 0, next_p = 0, line = 0;
			char char_of_pointer = 0;
			for (line = (unsigned)f_result; line <=(unsigned) l_result; line++) {
				next_p = line;
				while (true) {
					current_p = next_p;
					rank_b_val=rankB(current_p);
					char_of_pointer = getCharAtS(rank_b_val - 1);
					next_p = backwordDecode(current_p,char_of_pointer,rank_b_val);
					if (char_of_pointer == '[') {
						result--;
						break;
					}
					else if (char_of_pointer == ']' || !isdigit(char_of_pointer)) {
						break;
					}
				}
			}
			cout<<result<<endl;
		}
	}
}


void findAllUniqueMatch(unsigned f_result, unsigned l_result) {
	unsigned int current_p = 0, next_p = 0, line = 0, status = 0;
	char char_of_pointer = 0;
	for (line = f_result; line <= l_result; line++) {
		next_p = line;
		string index_str;
		string tmp;
		status = 0;
		prevPosTable[next_p] = true;
		while (true) {
			current_p = next_p;
			rank_b_val=rankB(current_p);
			char_of_pointer = getCharAtS(rank_b_val - 1);
			next_p = backwordDecode(current_p,char_of_pointer,rank_b_val);
			if (prevPosTable.find(next_p) != prevPosTable.end()) {
				//cout<<next_p<<endl;
				//cout<<char_of_pointer<<endl;
				break;
			}
			//cout<<char_of_pointer<<endl;
			if (status == 0 && char_of_pointer == '[') {
				break;
			}
			if (status == 0 && char_of_pointer == ']') {
				status = 1;
				index_str = "";
				tmp = "";
				prevPosTable[current_p] = true;
			}
			if (status == 1) {
				index_str += char_of_pointer;
				if (char_of_pointer == '[') {
					//cout<<index_str<<endl;
					for (s = index_str.length() - 1; s > 1; s--) {
						tmp += index_str[s - 1];
					}
					//cout<<stoi(tmp)<<endl;
					//mappingTable[stoi(tmp)]=true;
					mappingIndex.push_back(stoi(tmp));
					break;
				}

			}
		}
	}
}

void searchForR(string target) {
	backwardSearch(target, f_result, l_result);

	if (l_result - f_result + 1 <= 0) {
		return;
	}
	findAllUniqueMatch(f_result, l_result);
	cout << mappingIndex.size() << endl;
}

void sortArray(vector<int> &arr){
	int tmp=0,min=0;
	for(i=0;i<arr.size();i++){
		min=arr[i];
		tmp=i;
		for(j=i;j<arr.size();j++){
			if(arr[j]<min){
				min=arr[j];
				tmp=j;
			}
		}
		if(min<arr[i]){
			arr[tmp]=arr[i];
			arr[i]=min;
		}
	}
}

void searchForA(string target) {
	backwardSearch(target, f_result, l_result);
	if (l_result - f_result + 1 <= 0) {
		return;
	}
	findAllUniqueMatch(f_result, l_result);
	sortArray(mappingIndex);
	for (i = 0; i < mappingIndex.size(); i++) {
		cout << "[" << mappingIndex[i] << "]" << endl;
	}
}

void printPreviousContent(unsigned int start_p) {
	unsigned int current_p = 0, next_p = 0;
	char char_of_pointer = 0;
	next_p = start_p;
	string index_str;
	string tmp;
	index_str = "";
	tmp = "";
	while (true) {
		current_p = next_p;
		rank_b_val=rankB(current_p);
		char_of_pointer = getCharAtS(rank_b_val - 1);
		next_p = backwordDecode(current_p,char_of_pointer,rank_b_val);
		if (char_of_pointer == ']') {
			for (s = index_str.length(); s > 0; s--) {
				tmp += index_str[s - 1];
			}
			cout<<tmp<<endl;
			break;
		}else{
			index_str+=char_of_pointer;
		}
	}
}

void searchForN(string target) {
	if(!isNum(target)){
		return;
	}
	int target_num = stoi(target) + 1;
	string str = "";
	string prev_str = "[" + target + "]";
	backwardSearch(prev_str, f_result, l_result);
	if (l_result - f_result + 1 <= 0) {
		return;
	}
	while (true) {
		str = "[" + to_string(target_num) + "]";
		//cout<<str<<endl;
		backwardSearch(str, f_result, l_result);
		if (l_result - f_result + 1 > 0) {
			printPreviousContent(f_result);
			break;
		}
		target_num++;
		if ((unsigned int)target_num > count_of_s) {
			break;
		}
	}

	//cout<<target_num<<endl;
}


void readSB(string &fileName) {
	bbFN = fileName + ".bb";
	sFN = fileName + ".s";
	bFN = fileName + ".b";
	sp = fopen(sFN.c_str(), "r");
	bp = fopen(bFN.c_str(), "r");
	bbp = fopen(bbFN.c_str(), "r");
	if (!sp || !bp) {
		cout << fileName + ".s/.b not exists!" << endl;
	}
	fseek(bp,0L,SEEK_END);
	long bFileSize=ftell(bp);
	rewind(bp);
	fseek(sp,0L,SEEK_END);
	//long sFileSize=ftell(sp);
	rewind(sp);

	if(bFileSize<102400){
		SECTIONSIZE=128;
		CHARSECTIONSIZE=1024;
		BIT_SECTION_SIZE_OF_CHAR=16;
	}
	else if(bFileSize>512000 && bFileSize<=1024000){
		SECTIONSIZE*=2;
		CHARSECTIONSIZE=4096;
		BIT_SECTION_SIZE_OF_CHAR*=2;
	}else if(bFileSize>1024000 && bFileSize<=2048000){
		SECTIONSIZE*=4;
		CHARSECTIONSIZE=10240;
		BIT_SECTION_SIZE_OF_CHAR*=4;
	}
	else if(bFileSize>2048000 && bFileSize<=4096000){
		MAXSIZE*=2;
		SECTIONSIZE*=8;
		CHARSECTIONSIZE=20480;
		BIT_SECTION_SIZE_OF_CHAR*=8;
	}
	else if(bFileSize>4096000){
		MAXSIZE*=8;
		SECTIONSIZE*=16;
		CHARSECTIONSIZE*=2;
		CHARSECTIONSIZE=CHAR_SECTION_SIZE;
		BIT_SECTION_SIZE_OF_CHAR*=16;
	}


	s_buffer = new char[MAXSIZE];
	b_buffer = new char[MAXSIZE];
	bb_buffer = new char[MAXSIZE];
	//cout<<select_s.capacity()<<endl;
	// rank_s select_s
	while (!feof(sp)) {
		//cout<<s_section_count<<endl;
		current_s_buffer_size = (unsigned int) fread(s_buffer, 1, MAXSIZE, sp);
		for (i = 0; i < current_s_buffer_size; i++) {
			lens_table[(int) s_buffer[i]]++;
			if ((i + 1) % CHARSECTIONSIZE == 0) {
				for (j = 0; j < CHARSCALE; j++) {
					//cout<<select_s.size()<<","<<lens_table[j]<<endl;
					select_s.push_back(lens_table[j]);
				}
				s_section_count++;
			}
		}
		count_of_s += current_s_buffer_size;
	}

	//c_table
	unsigned int prev_chars_count = 0;
	for (i = 1; i < CHARSCALE; i++) {
		cs_table[i] = prev_chars_count;
		if (lens_table[i] > 0) {
			//cout<<(char)i<<prev_chars_count<<endl;
		}
		prev_chars_count += lens_table[i];
	}
	//rank_b select_b
	unsigned int b_count = 0;
	while (!feof(bp)) {
		current_b_buffer_size = (unsigned int) fread(b_buffer, 1, MAXSIZE, bp);
		//b_count : how many 1 in b
		for (i = 0; i < current_b_buffer_size; i++) {
			for (j = 0; j < 8; j++) {
				if (b_buffer[i] & (128 >> j)) {
					b_count++;
					/*
					if ((b_count) % SECTIONSIZE == 0) {
						//cout<<char_b_count*8+8*i+j<<endl;
						select_b.push_back((char_b_count + i) * 8 + j);
						select_b_section_count++;
					}*/
					if (b_count == count_of_s + 1) {
						suffix_b1_start = rank_b_section_count * SECTIONSIZE + (i * 8 + j) % SECTIONSIZE;
						break;
					}
				}
			}
			if ((i + 1) % BIT_SECTION_SIZE_OF_CHAR == 0) {
				rank_b.push_back(b_count);
				rank_b_section_count++;
			}
		}
		char_b_count += current_b_buffer_size;
		count_of_b = b_count;
	}
	//cout<<"b scaned."<<endl;
	if (!bbp) {
		bb_buffer_for_generate = new char[BB_BUFFER_SIZE];
		generateBB();
	} else {
		constructBBIndex();
	}
	/*
	for(i=0;i<=count_bb_1;i++){
		cout<<i<<"   "<<select_bb[i/SECTIONSIZE]<<"   "<<selectBBForSearch(i)<<endl;
	}*/
}

int main(int argc, char *argv[]) {
	if (argc < 4) {

	}

	std::string fileName = argv[2];
	std::string mode = argv[1];
	std::string target = argv[4];
	init();
	readSB(fileName);
	if (target[0] == '"' && target[target.length() - 1] == '"') {
		//cout<<"find \" "<<endl;
		target = target.substr(1, target.length() - 2);
	}

	//after read , Assume that all S B and BB is good.
	if (mode == "-m") {
		searchForTimes(target);
	} else if (mode == "-r") {
		searchForR(target);
	} else if (mode == "-a") {
		searchForA(target);
	} else if (mode == "-n") {
		searchForN(target);
	}


	if (access(argv[3], 00) < 0) {
		mkdir(argv[3], 0777);
	}


	return 0;
}