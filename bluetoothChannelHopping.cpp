#include <stdio.h>
#include <stdint.h>
#include "BluetoothWhitening.h"
#include "LinearFeedbackShiftRegister.h"

#include <vector>
#include <string>

#ifndef _WIN32
int fopen_s(FILE** pFile, const char *filename, const char *mode)
{
    (*pFile) = fopen(filename, mode);
    if(*pFile != nullptr)
    {
        return 0;
    }
    else
    {
        return errno;
    }
}

#define fscanf_s    fscanf
#endif

void TestHop(uint8_t mode, uint32_t address, uint32_t clkStart, uint32_t iterations);

void printhelp(const char* exeName)
{
    printf("%s -m <mode> -a <address> -c <clk>\n", exeName);
    printf("mode: 0 - Page Scan Inquiry Scan\n");
    printf("address: UAP and LAP hex\n");
    printf("clk: Estimated target clk.\n");
    printf("Example: %s -m 0 -a 01020304 -c 00000000 \n", exeName);
    printf("Output: \n");
//    printf("%s\n", exeName);
}

std::vector<uint8_t> testData;
std::vector<uint8_t> expectedData;
std::vector<uint8_t> dataOut;
uint8_t mode = 0;
uint32_t address = 0;
uint32_t clk = 0;
uint32_t iterations = 1;
uint32_t temp = 0;
bool testResults = false;
int unitTestIndex = -1;
int unitTestPassed = 0;

static void parseArgs(std::vector<std::string> args)
{
    testData.clear();
    expectedData.clear();
    dataOut.clear();
    mode = 0;
    address = 0;
    clk = 0;
    temp = 0;
    testResults = false;

    for (size_t i = 1; i < args.size(); i++)
    {
        char* endPtr = nullptr;

        if(args[i] == "-m")
        {
            i++;
            if(i < args.size())
            {
                mode = strtol(args[i].c_str() , &endPtr, 10);
                if (endPtr != args[i].c_str())
                {
                    switch(mode)
                    {
                    case 0:
                        break;
                    default:
                        printf("Invalid mode %u\n", mode);
                        printhelp(args[0].c_str());
                        exit(-1);
                    }
                }
            }
        }
        else if (args[i] == "--a")
        {
            i++;
            if(i < args.size())
            {
                address = strtol(args[i].c_str() , &endPtr, 16);
                if (endPtr == args[i].c_str())
                {
                    printf("Unable to parse address %s\n", args[i].c_str());
                    exit(-1);
                }
            }
        }
        else if (args[i] == "--c")
        {
            i++;
            if(i < args.size())
            {
                clk = strtol(args[i].c_str() , &endPtr, 16);
                if (endPtr == args[i].c_str())
                {
                    printf("Unable to parse clock %s\n", args[i].c_str());
                    exit(-1);
                }
            }
        }
        else if (args[i] == "--i")
        {
            i++;
            if(i < args.size())
            {
                iterations = strtol(args[i].c_str() , &endPtr, 10);
                if (endPtr == args[i].c_str())
                {
                    printf("Unable to parse iterations %s\n", args[i].c_str());
                    exit(-1);
                }
            }
        }
    }
}


int main(int argc, const char* argv[])
{
    std::vector<std::string> args;

    for (int i = 0; i < argc; i++)
    {
        args.push_back(argv[i]);
    }

    parseArgs(args);

//    if (unitTestIndex >= 0)
//    {
//        iterations = unitTests.size();
//    }

//    for (int i = 0; i < iterations; i++)
//    {
//        if (unitTestIndex >= 0)
//        {
//            args.clear();
//            char* tempString = new char[unitTests[unitTestIndex].size()];
//
//            int tempIndex = 0;
//            for (size_t i = 0; i < unitTests[unitTestIndex].size(); i++)
//            {
//                char c = unitTests[unitTestIndex][i];
//                if (c != ' ')
//                {
//                    tempString[tempIndex++] = c;
//                }
//                else
//                {
//                    tempString[tempIndex] = 0;
//                    args.push_back(tempString);
//                    tempIndex = 0;
//                }
//            }
//            parseArgs(args);
//            delete[] tempString;
//        }
//        if (testData.size() < 3)
//        {
//            printf("Insufficient data for test. Bluetooth header is 18 bits, user must supply at least 3 bytes of data!\n");
//            printhelp(argv[0]);
//            exit(-3);
//        }
        TestHop(mode, address, clk, iterations);

//        if (testResults)
//        {
//            uint32_t dataMatch = 0;
//            uint32_t dataFail = 0;
//            uint32_t dataTotal = 0;
//            if (dataOut.size() != expectedData.size())
//            {
//                printf("Size mismatch in dataOut! Expected %4zu Actual %4zu\n", expectedData.size(), dataOut.size());
//                dataFail++;
//            }
//
//            for (size_t i = 0; i < expectedData.size(); i++)
//            {
//                dataTotal++;
//                if (dataOut.size() >= i)
//                {
//                    if (expectedData[i] == dataOut[i])
//                    {
//                        dataMatch++;
//                    }
//                    else
//                    {
//                        printf("Mismatch in dataOut at index %4zu! Expected %02X Actual %02X\n", i, expectedData[i], dataOut[i]);
//                        dataFail++;
//                    }
//                }
//            }
//            printf("Matched %4u of %4u failed %4u, test %s\n", dataMatch, dataTotal, dataFail, dataFail == 0 ? "Passed" : "Failed");
//            unitTestPassed += dataFail == 0 ? 1 : 0;
//        }
//        unitTestIndex++;
//    }
//    if (testResults)
//    {
//        printf("\nPassed %4u of %4u tests! Unittest complete!\n", unitTestPassed, unitTestIndex);
//    }
}

uint8_t SelectionKernel(uint8_t X, uint8_t A, uint8_t B, uint8_t C, uint16_t D, uint8_t E, uint8_t F, uint8_t Y1, uint8_t Y2);

uint8_t GetBit(uint32_t source, uint8_t index, uint8_t outIndex)
{
    uint8_t returnVal = (source >> index) & 1;
    return returnVal << outIndex;
}
void TestHop(uint8_t mode, uint32_t address, uint32_t clkStart, uint32_t iterations)
{
    // Page, Page Response
    // A23_0 = LAP of device being paged
    // A27_24 = UAP3_0 of the device being paged
    // Inquiry
    // A23_0 = GIAC(0x9E8B33)
    // A27_24 = DCI(0x00)
    // Basic channel, Adapted channel
    // A23_0 = LAP of the Central
    // A27_24 = UAP3_0 of the Central
    // Page scan,      Generalized Page Scan,               Inquiry Scan, Generalized Inquiry Scan
    // X = CLKN16_12 , (CLKN16_12 + interlaceOffset) % 32 ,      Xir4_0 , (Xir4_0 + interlaceOffset) % 32
    // Y1 = 0
    // Y2 = 0
    // A  = A27_23
    // B  = A22_19
    // C  = A8,6,4,2,0
    // D  = A18_10
    // E  = A13,11,9,7,5,3,1
    // F  = 0
    // F' = n/a
//    uint32_t address = 0x00000000;
//    uint32_t address = 0x2A96EF25;
//    uint32_t address = 0x6587CBA9;
    // A9 -> 10101 -> 0x15
    // 96 -> 0010 -> 0x02
    // F25 -> 10011 -> 0x13
    // 6EF -> 110111011 -> 0x1BB
    // EF25 -> 1110100 -> 0x74
    uint8_t koffset = 24;
    uint8_t knudge = 0;
    uint32_t endClock = clkStart + iterations * 0x1000;
    for(uint32_t clk = clkStart; clk < endClock; clk += 0x1000)
    {
        uint32_t clk16_12 = (clk >> 12) & 0x1F;
        uint32_t clk4_20 = ((clk >> 1) & 0x1E) | (clk & 1);
        uint32_t clk2 = 0;//(clk16_12 + koffset + knudge + (clk4_20 - clk16_12 + 32) & 0xF) & 0x1F;
        uint8_t X = ((clk >> 12) + clk2) & 0x1F ;
        uint8_t A = (address >> 23) & 0x1F;
        uint8_t B = (address >> 19) & 0xF;
        uint8_t C = (GetBit(address, 8, 4) | GetBit(address, 6, 3) | GetBit(address, 4, 2) | GetBit(address, 2, 1) | GetBit(address, 0, 0))  & 0x1F;
        uint16_t D = (address >> 10) & 0x1FF;
        uint8_t E = (GetBit(address, 13, 6) | GetBit(address, 11, 5) | GetBit(address, 9, 4) | GetBit(address, 7, 3) | GetBit(address, 5, 2) | GetBit(address, 3, 1) | GetBit(address, 1, 0))  & 0x7F;
        uint8_t F = 0;
        uint8_t Y1 = 0;
        uint8_t Y2 = 0;
        uint8_t channel = SelectionKernel(X, A, B, C, D, E, F, Y1, Y2);
        printf("address %08X clk %08X channel %2u\n", address, clk, channel);

        if(clk + 0x1000 >= endClock)
        {
            printf("X %08X A %08X B %08X C %08X D %08X E %08X F %08X ADDR %08X\n", X, A, B, C, D, E, F, address);
        }
    }


    //        Page,      Inquiry
    // X =        Xp4_0, Xi4_0
    // Y1 =       CLKE1, CLKN1
    // Y2 = 32 x  CLKE1, 32 x CLKN1
    // A  = A27_23
    // B  = A22_19
    // C  = A8,6,4,2,0
    // D  = A18_10
    // E  = A13,11,9,7,5,3,1
    // F  = 0
    // F' = n/a
    // Central Page Resp, Peripheral Page Resp, Inquiry Respo
    // X =      Xprc4_0,    Xprp4_0,              Xir4_0
    // Y1 =       CLKE1,      CLKN1,                   1
    // Y2 = 32 x  CLKE1, 32 x CLKN1,               32 x 1
    // A  = A27_23
    // ;B  = A22_19
    // C  = A8,6,4,2,0
    // D  = A18_10
    // E  = A13,11,9,7,5,3,1
    // F  = 0
    // F' = n/a
    // Connection State
    // X = CLK6_2
    // Y1 = CLK1
    // Y2 = 32 x CLK1
    // A  = A27_23 ^ CLK25_21
    // B  = A22_19
    // C  = A8,6,4,2,0 ^ CLK20_16
    // D  = A18_10 ^ CLK15_7
    // E  = A13,11,9,7,5,3,1
    // F  = 16 x CLK27_7 % 79
    // F' = 16 x CLK27_7 % N

    // Page hopping Sequence:
    // Xp = [CLKE16_13 + koffset + knudge + (CLKE4_2,0 - CLKE16_12 + 32) % 16] ^ 32
    // koffset = 24 // A-train, 8 // B-train
    // knudge = 0 // 1st 2xNpage repetitions, Even // > 2xNpage

}



uint8_t SelectionKernel(uint8_t X, uint8_t A, uint8_t B, uint8_t C, uint16_t D, uint8_t E, uint8_t F, uint8_t Y1, uint8_t Y2)
{
    uint8_t butterFlyLut[] =
    {//       PAB
        0, // 000 -> 00
        1, // 001 -> 01
        2, // 010 -> 10
        3, // 011 -> 11
        0, // 100 -> 00
        2, // 101 -> 10
        1, // 110 -> 01
        3  // 111 -> 11
    };
    uint8_t xA, xB, xCD, xEF;
    xA = (X+A) & 0x1F;
    xB = xA ^ (B & 0xF);

    uint8_t perm[14] = {0};

    for(int i = 0 ; i < 9; i++)
    {
        perm[i] = (D >> i) & 1;
    }

    for(int i = 9 ; i < 14; i++)
    {
        perm[i] = ((C >> (i-9)) & 1) ^ Y1;
    }

    uint8_t z0 = xB & 1;
    uint8_t z1 = (xB>>1) & 1;
    uint8_t z2 = (xB>>2) & 1;
    uint8_t z3 = (xB>>3) & 1;
    uint8_t z4 = (xB>>4) & 1;

//    printf("z0 %u z1 %u z2 %u z3 %u z4 %u\n", z0, z1, z2, z3, z4);
    uint8_t p12Index = (perm[12] & 1) << 2 | z0 << 1 | z3;
    uint8_t p13Index = (perm[13] & 1) << 2 | z1 << 1 | z2;

    z0 = (butterFlyLut[p12Index] >> 1) & 1;
    z1 = (butterFlyLut[p13Index] >> 1) & 1;
    z3 = (butterFlyLut[p12Index] >> 0) & 1;
    z2 = (butterFlyLut[p13Index] >> 0) & 1;

//    printf("z0 %u z1 %u z2 %u z3 %u z4 %u p12 %u %u p13 %u %u\n", z0, z1, z2, z3, z4, perm[12], p12Index, perm[13], p13Index);

    uint8_t p10Index = (perm[10] & 1) << 2 | z2 << 1 | z4;
    uint8_t p11Index = (perm[11] & 1) << 2 | z1 << 1 | z3;

    z2 = (butterFlyLut[p10Index] >> 1) & 1;
    z1 = (butterFlyLut[p11Index] >> 1) & 1;
    z4 = (butterFlyLut[p10Index] >> 0) & 1;
    z3 = (butterFlyLut[p11Index] >> 0) & 1;

//    printf("z0 %u z1 %u z2 %u z3 %u z4 %u p10 %u %u p11 %u %u\n", z0, z1, z2, z3, z4, perm[10], p10Index, perm[11], p11Index);

    uint8_t p8Index = (perm[8] & 1) << 2 | z1 << 1 | z4;
    uint8_t p9Index = (perm[9] & 1) << 2 | z0 << 1 | z3;

    z1 = (butterFlyLut[p8Index] >> 1) & 1;
    z0 = (butterFlyLut[p9Index] >> 1) & 1;
    z4 = (butterFlyLut[p8Index] >> 0) & 1;
    z3 = (butterFlyLut[p9Index] >> 0) & 1;

//    printf("z0 %u z1 %u z2 %u z3 %u z4 %u p8 %u %u p9 %u %u\n", z0, z1, z2, z3, z4, perm[8], p8Index, perm[9], p9Index);

    uint8_t p6Index = (perm[6] & 1) << 2 | z0 << 1 | z2;
    uint8_t p7Index = (perm[7] & 1) << 2 | z3 << 1 | z4;

    z0 = (butterFlyLut[p6Index] >> 1) & 1;
    z3 = (butterFlyLut[p7Index] >> 1) & 1;
    z2 = (butterFlyLut[p6Index] >> 0) & 1;
    z4 = (butterFlyLut[p7Index] >> 0) & 1;

//    printf("z0 %u z1 %u z2 %u z3 %u z4 %u p6 %u %u p7 %u %u\n", z0, z1, z2, z3, z4, perm[6], p6Index, perm[7], p7Index);

    uint8_t p4Index = (perm[4] & 1) << 2 | z0 << 1 | z4;
    uint8_t p5Index = (perm[5] & 1) << 2 | z1 << 1 | z3;

    z0 = (butterFlyLut[p4Index] >> 1) & 1;
    z1 = (butterFlyLut[p5Index] >> 1) & 1;
    z4 = (butterFlyLut[p4Index] >> 0) & 1;
    z3 = (butterFlyLut[p5Index] >> 0) & 1;

//    printf("z0 %u z1 %u z2 %u z3 %u z4 %u p4 %u %u p5 %u %u\n", z0, z1, z2, z3, z4, perm[4], p4Index, perm[5], p5Index);

    uint8_t p2Index = (perm[2] & 1) << 2 | z1 << 1 | z2;
    uint8_t p3Index = (perm[3] & 1) << 2 | z3 << 1 | z4;

    z1 = (butterFlyLut[p2Index] >> 1) & 1;
    z3 = (butterFlyLut[p3Index] >> 1) & 1;
    z2 = (butterFlyLut[p2Index] >> 0) & 1;
    z4 = (butterFlyLut[p3Index] >> 0) & 1;

//    printf("z0 %u z1 %u z2 %u z3 %u z4 %u p2 %u %u p3 %u %u\n", z0, z1, z2, z3, z4, perm[2], p2Index, perm[3], p3Index);

    uint8_t p0Index = (perm[0] & 1) << 2 | z0 << 1 | z1;
    uint8_t p1Index = (perm[1] & 1) << 2 | z2 << 1 | z3;

    z0 = (butterFlyLut[p0Index] >> 1) & 1;
    z2 = (butterFlyLut[p1Index] >> 1) & 1;
    z1 = (butterFlyLut[p0Index] >> 0) & 1;
    z3 = (butterFlyLut[p1Index] >> 0) & 1;

//    printf("z0 %u z1 %u z2 %u z3 %u z4 %u p0 %u %u p1 %u %u\n", z0, z1, z2, z3, z4, perm[0], p0Index, perm[1], p1Index);

    xCD = z0 | z1 << 1 | z2 << 2 | z3 << 3 | z4 << 4;

    xEF = (xCD + E + F + Y2) % 79;
//    printf("xA %08X xB %08X xCD %08X xEF %08X | ", xA, xB, xCD, xEF);
    if(xEF < 40)
    {
        return xEF * 2;
    }
    else
    {
        return ((xEF-40) * 2) + 1;
    }

//    return xEF;
}

