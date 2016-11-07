
//#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <algorithm>


using namespace std;

const int block_size_M = 6;
const int block_size_B = 3;

void CreateTestFile();

void
mergeRuns(ifstream &infile1, ifstream &infile2, ofstream &outfile, long run1_start, long run2_start, long run2_end);

void coutFile(char *filename);


int main(int argc, char *argv[])
{

    const auto startTime = std::clock();

    CreateTestFile();

    long N;

    ifstream infile("input.bin", ios::in | ios::binary);
    ofstream tempfile("temp1.bin", ios::out | ios::binary);


    infile.read(reinterpret_cast<char *>(&N), 8);
    tempfile.write(reinterpret_cast<char *>(&N), 8);

    long init_offset = 8;
//======================================================================================================================
// Phase 1
//======================================================================================================================
    vector<double> bufferM(block_size_M);

    long runs = ceil((double) N / block_size_M);
    long read_size_M = block_size_M * sizeof(double);

    for (int j = 0; j < runs; ++j)
    {
        if (N - j * block_size_M < block_size_M)
        {
            read_size_M = (N - j * block_size_M) * sizeof(double);
            bufferM.resize(read_size_M / sizeof(double));
        }

        infile.read((char *) &bufferM[0], read_size_M);

        sort(bufferM.begin(), bufferM.end());

        tempfile.write((char *) &bufferM[0], read_size_M);
    }

    bufferM.clear();
    tempfile.flush();
    tempfile.close();
    infile.close();

    //coutFile((char *) "temp1.bin");

//======================================================================================================================
// Phase 2
//======================================================================================================================

    if (N > block_size_M)
    {

        ifstream tempfile1;
        ifstream tempfile2;
        ofstream outfile1;

        read_size_M = block_size_M * sizeof(double);

        long levels = (long) ceil(log2(runs));


        for (int i = 0; i < levels; ++i)
        {
            if (i % 2 == 0)
            {
                tempfile1.open("temp1.bin", ios::in | ios::binary);
                tempfile2.open("temp1.bin", ios::in | ios::binary);
                outfile1.open("temp2.bin", ios::out | ios::binary | ios::trunc);
            } else
            {
                tempfile1.open("temp2.bin", ios::in | ios::binary);
                tempfile2.open("temp2.bin", ios::in | ios::binary);
                outfile1.open("temp1.bin", ios::out | ios::binary | ios::trunc);
            }

            outfile1.write(reinterpret_cast<char *>(&N), 8);

            for (long k = init_offset; k < N * sizeof(double) + init_offset; k += 2 * read_size_M)
            {
                long run1_start = k;
                long run2_start = k + read_size_M;
                long run2_end = k + 2 * read_size_M;

                if ((N * sizeof(double) + init_offset) - k < read_size_M)
                {
                    run2_start = k;
                }

                if ((N * sizeof(double) + init_offset) - run2_start < read_size_M)
                {
                    run2_end = (N * sizeof(double) + init_offset);
                }

                mergeRuns(tempfile1, tempfile2, outfile1, run1_start, run2_start, run2_end);
            }
            read_size_M *= 2;

            tempfile1.close();
            tempfile2.close();
            outfile1.close();
        }

        //coutFile((char *) "temp1.bin");
        //coutFile((char *) "temp2.bin");
        levels % 2 == 0 ? rename("temp1.bin", "output.bin") : rename("temp2.bin", "output.bin");
        coutFile((char *) "output.bin");

    } else
    {
        rename("temp1.bin", "output.bin");
        coutFile((char *) "output.bin");
    }

    const auto endTime = std::clock();
    std::cout << endl << "done in  " << double(endTime - startTime) / CLOCKS_PER_SEC << '\n';
    //getchar();

    return 0;
}


void mergeRuns(ifstream &infile1, ifstream &infile2, ofstream &outfile, long run1_start, long run2_start, long run2_end)
{

    long blk_size_B1 = block_size_B;
    long blk_size_B2 = block_size_B;

    long read_size_B1 = block_size_B * sizeof(double);
    long read_size_B2 = block_size_B * sizeof(double);

    long read_blk1_offset = run1_start;
    long read_blk2_offset = run2_start;

    vector<double> bufferBr1(block_size_B);
    vector<double> bufferBr2(block_size_B);

    infile1.seekg(run1_start, ios::beg);
    infile1.read((char *) &bufferBr1[0], read_size_B1);

    infile2.seekg(run2_start, ios::beg);
    infile2.read((char *) &bufferBr2[0], read_size_B2);

    vector<double> bufferBw(block_size_B);

    int p1 = 0, p2 = 0, po = 0;

    while (read_blk1_offset < run2_start && read_blk2_offset < run2_end)
    {


        if (run2_end - read_blk2_offset < read_size_B2)
        {
            blk_size_B2 = (run2_end - read_blk2_offset) / sizeof(double);
            read_size_B2 = (run2_end - read_blk2_offset);
        }


        if (bufferBr1[p1] < bufferBr2[p2])
        {
            bufferBw[po] = bufferBr1[p1];
            p1++;

            if (p1 > blk_size_B1 - 1)
            {

                read_blk1_offset += read_size_B1;

                if (read_blk1_offset < run2_start)
                {
                    infile1.read((char *) &bufferBr1[0], read_size_B1);
                    p1 = 0;
                }
            }
        } else
        {
            bufferBw[po] = bufferBr2[p2];
            p2++;
            if (p2 > blk_size_B2 - 1)
            {
                read_blk2_offset += read_size_B2;

                if (read_blk2_offset < run2_end)
                {
                    infile2.read((char *) &bufferBr2[0], read_size_B2);
                    p2 = 0;
                }
            }
        }

        po++;
        if (po > blk_size_B1 - 1)
        {
            outfile.write((char *) &bufferBw[0], read_size_B1);
            po = 0;
        }
    }

    read_size_B1 = block_size_B * sizeof(double);
    read_size_B2 = block_size_B * sizeof(double);

    if (read_blk1_offset == run2_start)
    {

            for (int j = p2; j < blk_size_B2; ++j)
            {
                bufferBw[po] = bufferBr2[j];
                po++;
            }
            outfile.write((char *) &bufferBw[0], po*sizeof(double));
            read_blk2_offset += po*sizeof(double);


        for (long i = read_blk2_offset; i < run2_end; i += read_size_B2)
        {
            if (run2_end - read_blk2_offset < read_size_B2)
            {
                read_size_B2 = (run2_end - read_blk2_offset);
            }

            infile2.read((char *) &bufferBr2[0], read_size_B2);
            outfile.write((char *) &bufferBr2[0], read_size_B2);
        }
    } else if (read_blk2_offset == run2_end)
    {

            for (int j = p1; j < blk_size_B1; ++j)
            {
                bufferBw[po] = bufferBr1[j];
                po++;
            }
            outfile.write((char *) &bufferBw[0], po*sizeof(double));
            read_blk1_offset += po*sizeof(double);


        for (long i = read_blk1_offset; i < run2_start - read_size_B1; i += read_size_B1)
        {
            if (run2_start - read_blk1_offset < read_size_B1)
            {
                read_size_B1 = (run2_start - read_blk1_offset);
            }

            infile1.read((char *) &bufferBr1[0], read_size_B1);
            outfile.write((char *) &bufferBr1[0], read_size_B1);
        }
    }

    bufferBr1.clear();
    bufferBr2.clear();
    bufferBw.clear();
}

void coutFile(char *filename)
{
    cout << endl;
    ifstream infile(filename, ios::in | ios::binary);

    long n;
    infile.read(reinterpret_cast<char *>(&n), 8);
    vector<double> buffer(n, 0);
    infile.read((char *) &buffer[0], buffer.size() * sizeof(double));
    for (int i = 0; i < n; ++i)
    {
        cout << setiosflags(ios::fixed | ios::left) << setw(3) << buffer[i] << " ";
    }
    infile.close();
}

void CreateTestFile()
{
    const long n = 39;
    //srand(static_cast<unsigned int>(time(NULL)));

    vector<double> array(n); //{9,8,7,6,5,4,11,18,15};
    int c = n;
    for (int i = 0; i < n; ++i)
    {
        array[i] = rand() % n;
        c--;
    }

    for (int i = 0; i < n; ++i)
    {
        cout << setiosflags(ios::fixed | ios::left) << setw(3) << array[i] << " ";
    }

    ofstream out("input.bin", ios::out | ios::binary);

    out.write((char *) &n, sizeof(long));

    out.write((char *) &array[0], n * sizeof(long));

    out.close();

    sort (array.begin(),array.end());
    cout<<endl;
    for (int i = 0; i < n; ++i)
    {
        cout << setiosflags(ios::fixed | ios::left) << setw(3) << array[i] << " ";
    }
}
