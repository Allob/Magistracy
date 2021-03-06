//#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;

const long long block_size_M = 192 * 192;
const long long block_size_B = 64 * 64;

void mergeRuns(ifstream &infile1, ifstream &infile2, ofstream &outfile, long long run1_start, long long run2_start, long long run2_end);


int main(int argc, char *argv[])
{
	long long N;

	ifstream infile("input.bin", ios::in | ios::binary);
	ofstream tempfile("temp1.bin", ios::out | ios::binary);

	infile.read(reinterpret_cast<char *>(&N), sizeof(double));
	tempfile.write(reinterpret_cast<char *>(&N), sizeof(double));

	long long init_offset = sizeof(double);

	//======================================================================================================================
	// Phase 1
	//======================================================================================================================

	vector<double> bufferM(block_size_M);

	long long runs = ceil((double)N / block_size_M);
	long long read_size_M = block_size_M * sizeof(double);

	for (int j = 0; j < runs; ++j)
	{
		if (N - j * block_size_M < block_size_M)
		{
			read_size_M = (N - j * block_size_M) * sizeof(double);
			bufferM.resize(read_size_M / sizeof(double));
		}

		infile.read((char *)&bufferM[0], read_size_M);

		sort(bufferM.begin(), bufferM.end());

		tempfile.write((char *)&bufferM[0], read_size_M);
	}

	bufferM.clear();
	tempfile.close();
	infile.close();

	//======================================================================================================================
	// Phase 2
	//======================================================================================================================

	if (N > block_size_M)
	{

		ifstream tempfile1;
		ifstream tempfile2;
		ofstream outfile1;

		read_size_M = block_size_M * sizeof(double);

		long long levels = (long long)ceil(log2(runs));

		for (int i = 0; i < levels; ++i)
		{
			if (i % 2 == 0)
			{
				tempfile1.open("temp1.bin", ios::in | ios::binary);
				tempfile2.open("temp1.bin", ios::in | ios::binary);
				outfile1.open("temp2.bin", ios::out | ios::binary | ios::trunc);
			}
			else
			{
				tempfile1.open("temp2.bin", ios::in | ios::binary);
				tempfile2.open("temp2.bin", ios::in | ios::binary);
				outfile1.open("temp1.bin", ios::out | ios::binary | ios::trunc);
			}
			
			outfile1.write(reinterpret_cast<char *>(&N), sizeof(double));
			
			long long end_of_file = N * sizeof(double) + init_offset;
			
			for (long long offset = init_offset; offset < end_of_file; offset += 2 * read_size_M)
			{
				long long run1_start = offset;
				long long run2_start = run1_start + read_size_M;
				long long run2_end = run1_start + 2 * read_size_M;

				if (end_of_file - run1_start < read_size_M)
				{
					run2_start = run1_start;
				}

				if (end_of_file - run2_start < read_size_M)
				{
					run2_end = end_of_file;
				}

				if (run2_end == run2_start)
				{
					run2_start = run1_start;
				}

				mergeRuns(tempfile1, tempfile2, outfile1, run1_start, run2_start, run2_end);
			}
			read_size_M *= 2;

			tempfile1.close();
			tempfile2.close();
			outfile1.close();
		}

		levels % 2 == 0 ? rename("temp1.bin", "output.bin") : rename("temp2.bin", "output.bin");
	}
	else
	{
		rename("temp1.bin", "output.bin");
	}

	return 0;
}


void mergeRuns(ifstream &infile1, ifstream &infile2, ofstream &outfile, long long run1_start, long long run2_start, long long run2_end)
{

	long long blk_size_B1 = block_size_B;
	long long blk_size_B2 = block_size_B;

	long long read_blk_size1 = block_size_B * sizeof(double);
	long long read_blk_size2 = block_size_B * sizeof(double);

	long long read_blk_offset1 = run1_start;
	long long read_blk_offset2 = run2_start;

	if (run2_end - read_blk_offset2 < read_blk_size2)
	{
		blk_size_B2 = (run2_end - read_blk_offset2) / sizeof(double);
		read_blk_size2 = (run2_end - read_blk_offset2);
	}

	vector<double> bufferBr1(blk_size_B1);
	vector<double> bufferBr2(blk_size_B2);
	vector<double> bufferBw(blk_size_B1);

	infile1.seekg(read_blk_offset1, ios::beg);
	infile1.read((char *)&bufferBr1[0], read_blk_size1);

	infile2.seekg(read_blk_offset2, ios::beg);
	infile2.read((char *)&bufferBr2[0], read_blk_size2);


	long long p1 = 0, p2 = 0, po = 0;

	while (read_blk_offset1 < run2_start && read_blk_offset2 < run2_end)
	{

		if (run2_end - read_blk_offset2 < read_blk_size2)
		{
			blk_size_B2 = (run2_end - read_blk_offset2) / sizeof(double);
			read_blk_size2 = (run2_end - read_blk_offset2);
		}

		if (bufferBr1[p1] < bufferBr2[p2])
		{
			bufferBw[po] = bufferBr1[p1];
			p1++;

			if (p1 > blk_size_B1 - 1)
			{
				read_blk_offset1 += read_blk_size1;

				if (read_blk_offset1 < run2_start)
				{
					infile1.read((char *)&bufferBr1[0], read_blk_size1);
					p1 = 0;
				}
			}
		}
		else
		{
			bufferBw[po] = bufferBr2[p2];
			p2++;

			if (p2 > blk_size_B2 - 1)
			{
				read_blk_offset2 += read_blk_size2;

				if (read_blk_offset2 < run2_end)
				{
					infile2.read((char *)&bufferBr2[0], read_blk_size2);
					p2 = 0;
				}
			}
		}

		po++;
		if (po > blk_size_B1 - 1)
		{
			outfile.write((char *)&bufferBw[0], read_blk_size1);
			po = 0;
		}
	}


	if (read_blk_offset1 == run2_start)
	{
		long long i;
		for (i = p2; i < blk_size_B2; ++i)
		{
			bufferBw[po] = bufferBr2[i];
			po++;
			if (po > blk_size_B1 - 1) break;
		}
		outfile.write((char *)&bufferBw[0], po * sizeof(double));


		if (++i < blk_size_B2)
		{
			outfile.write((char *)&bufferBr2[i], (blk_size_B2 - i) * sizeof(double));
		}

		for (long long j = read_blk_offset2 + read_blk_size2; j < run2_end; j += read_blk_size2)
		{
			if (run2_end - j < read_blk_size2)
			{
				read_blk_size2 = run2_end - j;
			}
			infile2.read((char *)&bufferBr2[0], read_blk_size2);
			outfile.write((char *)&bufferBr2[0], read_blk_size2);
		}
	}
	else if (read_blk_offset2 == run2_end)
	{
		long long i;
		for (i = p1; i < blk_size_B1; ++i)
		{
			bufferBw[po] = bufferBr1[i];
			po++;
			if (po > blk_size_B1 - 1) break;
		}

		outfile.write((char *)&bufferBw[0], po * sizeof(double));


		if (++i < blk_size_B1)
		{
			outfile.write((char *)&bufferBr1[i], (blk_size_B1 - i) * sizeof(double));
		}

		for (long long j = read_blk_offset1 + read_blk_size1; j < run2_start; j += read_blk_size1)
		{
			if (run2_start - j < read_blk_size1)
			{
				read_blk_size1 = run2_start - j;
			}
			infile1.read((char *)&bufferBr1[0], read_blk_size1);
			outfile.write((char *)&bufferBr1[0], read_blk_size1);
		}
	}
}
