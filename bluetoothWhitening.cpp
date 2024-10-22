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

void TestHop();

const char* unitTestWhitening =
    "unitTestWhitening "
    "60 "
    "10 D0 00 C1 9E 81 3F AB 74 72 97 86 5D 64 0C 01 2A C2 CB E7 09 "
    "--e "
    "6F 0C 00 8B C1 04 C9 37 EE B3 41 43 19 44 55 DF CB 4D D0 42 A6 ";
class BluetoothWhitening
{
public:
    BluetoothWhitening(uint32_t clock)
        :lsfr(7, 0x40 | ((clock >> 1) & 0x3F))
    {
        // poly from bluetooth spec for whitening
        lsfr.AddGaloisPoly(0x91);
        // standard generator, only including output of the final register
        lsfr.AddGeneratorPoly(0x40);
    }

    void WhitenData(std::vector<uint8_t>& dataIn, std::vector<uint8_t>& dataOut)
    {
        const size_t HEADER_SIZE_BITS = 18;
        const uint8_t BIT_MASK_TABLE[7] =
        {
            0x01,
            0x03,
            0x07,
            0x0F,
            0x1F,
            0x3F,
            0x7F,
        };
        dataOut.resize(dataIn.size());
        size_t dataIndex = 0;

//        printf("Header:\n");
        lsfr.Shift(HEADER_SIZE_BITS);
        auto whiteningCodeHeader = lsfr.GetDataOut(0);
        for (; dataIndex < HEADER_SIZE_BITS/8; dataIndex++)
        {
            dataOut[dataIndex] = dataIn[dataIndex] ^ whiteningCodeHeader[dataIndex];
//            printf("data[%2zu] %02X -> %02X\n", dataIndex, dataIn[dataIndex], dataOut[dataIndex]);
        }
        uint8_t lastByteBits = HEADER_SIZE_BITS & 0x7;
        if (lastByteBits != 0)
        {
            dataOut[dataIndex] = (dataIn[dataIndex] ^ whiteningCodeHeader[dataIndex]) & BIT_MASK_TABLE[lastByteBits - 1];
//            printf("data[%2zu] %02X -> %02X\n", dataIndex, dataIn[dataIndex], dataOut[dataIndex]);
        }
        dataIndex++;

//        printf("Data:\n");
        lsfr.ClearDataOut(0, false);
        lsfr.Shift(static_cast<uint32_t>((dataIn.size() - dataIndex) * 8));
        auto whiteningCodeData = lsfr.GetDataOut(0);
        for (int whiteningIndex = 0; dataIndex < dataIn.size(); dataIndex++, whiteningIndex++)
        {
            dataOut[dataIndex] = dataIn[dataIndex] ^ whiteningCodeData[whiteningIndex];
//            printf("data[%2zu] %02X -> %02X\n", dataIndex, dataIn[dataIndex], dataOut[dataIndex]);
        }
    }

private:

    LinearFeedbackShiftRegister lsfr;
};

const char* unitTestHec =
"unitTestHec "
"--hec "
"47 "
"00 23 01 "
"47 23 01 "
"00 24 01 "
"47 24 01 "
"00 25 01 "
"47 25 01 "
"00 26 01 "
"47 26 01 "
"00 27 01 "
"47 27 01 "
"00 1B 01 "
"47 1B 01 "
"00 1C 01 "
"47 1C 01 "
"00 1D 01 "
"47 1D 01 "
"00 1E 01 "
"47 1E 01 "
"00 1F 01 "
"47 1F 01 "
"--e "
"E1 06 32 D5 5A BD E2 05 8A 6D 9E 79 4D AA 25 C2 9D 7A F5 12 ";
class BluetoothHec
{
public:
    const uint32_t bluetoothHecPoly = 0x1A7;
    const uint32_t bluetoothHecRegCnt = 8;
    const uint32_t bluetoothHecPayloadBitCnt = 10;

    BluetoothHec(uint8_t uap)
        :lsfr(bluetoothHecRegCnt, uap)
    {
        // poly from bluetooth spec for whitening
        lsfr.AddGaloisPoly(bluetoothHecPoly);
        // standard generator, only including output of the final register
        lsfr.AddGeneratorPoly(1 << (bluetoothHecRegCnt - 1));
        lsfr.AddInputPoly(bluetoothHecPoly);
    }

    void CalcHec(uint8_t uap, std::vector<uint8_t>& dataIn, uint8_t& hecVal)
    {
        lsfr.Reset(uap);
        lsfr.Shift(bluetoothHecPayloadBitCnt, dataIn);

        hecVal = (uint8_t)lsfr.GetState();
    }

private:

    LinearFeedbackShiftRegister lsfr;
};


const char* unitTestCrc =
"unitTestCrc "
"--c "
"47 "
"4E 01 02 03 04 05 06 07 08 09 "
"--e "
"6D D2 ";
class BluetoothCrc
{
public:
    const uint32_t bluetoothCrcPoly = 0x11021;
    const uint32_t bluetoothCrcRegCnt = 16;

    BluetoothCrc(uint8_t uap)
        :lsfr(bluetoothCrcRegCnt, uap)
    {
        // poly from bluetooth spec for whitening
        lsfr.AddGaloisPoly(bluetoothCrcPoly);
        // standard generator, only including output of the final register
        lsfr.AddGeneratorPoly(1 << (bluetoothCrcRegCnt - 1));
        lsfr.AddInputPoly(bluetoothCrcPoly);
    }

    void CalcCrc(std::vector<uint8_t>& dataIn, uint16_t& crcVal)
    {
        lsfr.Shift(static_cast<uint32_t>(dataIn.size() * 8), dataIn);

        crcVal = (uint16_t)lsfr.GetState();
//        crcVal = lsfr.GetDataOut(0)[dataIn.size()] | lsfr.GetDataOut(0)[dataIn.size() + 1] << 8;
    }

private:

    LinearFeedbackShiftRegister lsfr;
};

const char* unitTestFec23 =
"unitTestFec23 "
"--f "
"00 "
"01 00 02 00 04 00 08 00 10 00 20 00 40 00 80 00 00 01 00 02 "
"--e "
"0B 16 07 0E 1C 13 0D 1A 1F 15 ";
class BluetoothFec23
{
public:
    const uint32_t bluetoothFec23Poly = 0x35;
    const uint32_t bluetoothFec23RegCnt = 5;
    const uint32_t bluetoothFec23PayloadBitCnt = 10;

    BluetoothFec23()
        :lsfr(bluetoothFec23RegCnt, 0)
    {
        // poly from bluetooth spec for whitening
        lsfr.AddGaloisPoly(bluetoothFec23Poly);
        // standard generator, only including output of the final register
        lsfr.AddGeneratorPoly(1 << (bluetoothFec23RegCnt - 1));
        lsfr.AddInputPoly(bluetoothFec23Poly);
    }

    void CalcParity(std::vector<uint8_t>& dataIn, uint8_t& parity)
    {
        lsfr.ClearDataOut(0);
        for (int i = 0; i < 10; i++)
        {
            lsfr.Shift(bluetoothFec23PayloadBitCnt, dataIn);

            parity = (uint8_t)lsfr.GetState();
            printf("bit %2u state %02X\n", i, parity);
        }
        //        crcVal = lsfr.GetDataOut(0)[dataIn.size()] | lsfr.GetDataOut(0)[dataIn.size() + 1] << 8;
    }

private:

    LinearFeedbackShiftRegister lsfr;
};

std::vector<std::string> unitTests =
{
    unitTestWhitening,
    unitTestHec,
    unitTestCrc,
    unitTestFec23
};

void printhelp(const char* exeName)
{
    printf("%s [BluetoothClk] [[testData] or [filename]]\n", exeName);
    printf("BluetoothClk: only bits 1 - 6 inclusive are used\n");
    printf("testData: space separated 2 digit hex bytes\n");
    printf("filename: the file can be text with space separated 2 digit hex bytes, or binary. Detection is automatic.\n");
    printf("Example: %s 60 10 D0 00 C1 9E 81 3F AB 74 72 97 86 5D 64 0C 01 2A C2 CB E7 09\n", exeName);
    printf("Output: \n");
//    printf("%s\n", exeName);
}

std::vector<uint8_t> testData;
std::vector<uint8_t> expectedData;
std::vector<uint8_t> dataOut;
uint8_t seed = 0;
uint32_t temp = 0;
bool seedByteParsed = false;
bool forcebinaryFile = false;
bool crcMode = false;
bool fecMode = false;
bool hecMode = false;
bool testResults = false;
bool hopTest = false;
int unitTestIndex = -1;
int unitTestPassed = 0;


static void parseData(const char* arg, std::vector<uint8_t>& data)
{
    char* endPtr = nullptr;

    temp = strtol(arg, &endPtr, 16);
    if (endPtr != arg)
    {
        data.push_back(temp & 0xFF);
    }
    else
    {
        size_t bytesRead = 0;
        if (forcebinaryFile == false)
        {
            FILE* dataFile = nullptr;
            fopen_s(&dataFile, arg, "rt");
            if (dataFile != nullptr)
            {
                int conversions = 1;

                while (conversions > 0 && feof(dataFile) == false)
                {
                    conversions = fscanf_s(dataFile, "%02X", &temp);
                    if (conversions > 0)
                    {
                        data.push_back(temp & 0xFF);
                        bytesRead++;
                    }
                    else
                    {
                        if (bytesRead == 0)
                        {
                            fclose(dataFile);
                            break;
                        }
                    }
                }
            }
        }

        if (bytesRead == 0)
        {
            FILE* dataFile = nullptr;
            fopen_s(&dataFile, arg, "rt");
            if (dataFile != nullptr)
            {
                fseek(dataFile, 0L, SEEK_END);
                size_t length = ftell(dataFile);
                fseek(dataFile, 0L, SEEK_SET);
                size_t startIndex = data.size();
                data.resize(startIndex + length);
                fread(&data[startIndex], 1, length, dataFile);
            }
        }

    }
}

static void parseArgs(std::vector<std::string> args)
{
    testData.clear();
    expectedData.clear();
    dataOut.clear();
    seed = 0;
    temp = 0;
    seedByteParsed = false;
    forcebinaryFile = false;
    crcMode = false;
    fecMode = false;
    hecMode = false;
    testResults = false;
    hopTest = false;

    for (size_t i = 1; i < args.size(); i++)
    {
        char* endPtr = nullptr;

        if(args[i] == "--hop")
        {
            TestHop();
            exit(0);
        }
        else if (args[i] == "--u")
        {
            unitTestIndex = 0;
            return;
        }
        else if (args[i] == "--hec")
        {
            hecMode = true;
        }
        else if (args[i] == "--e")
        {
            testResults = true;
        }
        else if (args[i] == "--b")
        {
            forcebinaryFile = true;
        }
        else if (args[i] == "--f")
        {
            fecMode = true;
        }
        else if (args[i] == "--c")
        {
            crcMode = true;
        }
        else if (seedByteParsed == false)
        {
            temp = strtol(args[i].c_str() , &endPtr, 16);
            if (endPtr != args[i].c_str())
            {
                if (temp >> 7 != 0)
                {
                    printf("Warning Bluetooth clock has too many bits! Only using lowest 7 bits!\n");
                }
                seedByteParsed = true;
                seed = temp;
            }
        }
        else
        {
            if (testResults)
            {
                parseData(args[i].c_str(), expectedData);
            }
            else
            {
                parseData(args[i].c_str(), testData);
            }
        }
    }
}

void TestHop();

int main(int argc, const char* argv[])
{
    int iterations = 1;
    std::vector<std::string> args;

    for (int i = 0; i < argc; i++)
    {
        args.push_back(argv[i]);
    }

    parseArgs(args);

    if (unitTestIndex >= 0)
    {
        iterations = unitTests.size();
    }

    for (int i = 0; i < iterations; i++)
    {
        if (unitTestIndex >= 0)
        {
            args.clear();
            char* tempString = new char[unitTests[unitTestIndex].size()];

            int tempIndex = 0;
            for (size_t i = 0; i < unitTests[unitTestIndex].size(); i++)
            {
                char c = unitTests[unitTestIndex][i];
                if (c != ' ')
                {
                    tempString[tempIndex++] = c;
                }
                else
                {
                    tempString[tempIndex] = 0;
                    args.push_back(tempString);
                    tempIndex = 0;
                }
            }
            parseArgs(args);
            delete[] tempString;
        }
        if (seedByteParsed == false)
        {
            printf("No valid Bluetooth clock value detected!\n");
            printhelp(argv[0]);
            exit(-2);
        }
        if (testData.size() < 3)
        {
            printf("Insufficient data for test. Bluetooth header is 18 bits, user must supply at least 3 bytes of data!\n");
            printhelp(argv[0]);
            exit(-3);
        }

        if (hecMode)
        {
            // --hec 47 00 23 01 47 23 01 00 24 01 47 24 01 00 25 01 47 25 01 00 26 01 47 26 01 00 27 01 47 27 01 00 1B 01 47 1B 01 00 1C 01 47 1C 01 00 1D 01 47 1D 01 00 1E 01 47 1E 01 00 1F 01 47 1F 01 --e E1 06 32 D5 5A BD E2 05 8A 6D 9E 79 4D AA 25 C2 9D 7A F5 12
            // --hec 47 00 23 01 --e E1
            uint8_t hecVal = 0;
            BluetoothHec hec(0);
            auto testDataIt = testData.begin();
            dataOut.clear();
            for (size_t i = 0; (i + 2) < testData.size(); i += 3, testDataIt+=3)
            {
                std::vector<uint8_t> fecData(testDataIt+1, testDataIt + 3);
                hec.CalcHec(testDataIt[0], fecData, hecVal);

                printf("uap %02X data %02X %02X hec %02X\n", testDataIt[0], fecData[0], fecData[1], hecVal);

                dataOut.push_back(hecVal);
            }
        }
        else if (crcMode)
        {
            // --c 47 4E 01 02 03 04 05 06 07 08 09 6D D2
            uint16_t crcVal;
            BluetoothCrc crcGen(seed);
            crcGen.CalcCrc(testData, crcVal);
            printf("uap %02X crc %04X\n", seed, crcVal);
            dataOut.resize(2);
            dataOut[0] = crcVal & 0xFF;
            dataOut[1] = (crcVal >> 8) & 0xFF;
        }
        else if (fecMode)
        {
            // --f 00 01 00 02 00 04 00 08 00 10 00 20 00 40 00 80 00 00 01 00 02
            uint8_t parity = 0;
            BluetoothFec23 fec;
            auto testDataIt = testData.begin();
            dataOut.clear();
            for (size_t i = 0; (i + 1) < testData.size(); i += 2)
            {
                std::vector<uint8_t> fecData(testDataIt, testDataIt + 2);
                testDataIt += 2;
                fec.CalcParity(fecData, parity);
                printf("parity %02X: ", parity);
                for (int i = 0; i < 10; i++)
                {
                    printf("%u", (fecData[i / 8] >> (i & 7)) & 1);
                }
                printf(" ");
                for (int i = 0; i < 5; i++)
                {
                    printf("%u", (parity >> i) & 1);
                }
                printf("\n");
                dataOut.push_back(parity);
            }
        }
        else
        {
            //60 10 D0 00 C1 9E 81 3F AB 74 72 97 86 5D 64 0C 01 2A C2 CB E7 09
            //   6F 0C 00 8B C1 04 C9 37 EE B4 41 43 19 44 55 DF CB 4D D0 42 A6
            BluetoothWhitening whitening(seed);
            whitening.WhitenData(testData, dataOut);

            for (size_t i = 0; i < dataOut.size(); i++)
            {
                printf("%02X ", dataOut[i]);
            }
            printf("\n");
        }
        if (testResults)
        {
            uint32_t dataMatch = 0;
            uint32_t dataFail = 0;
            uint32_t dataTotal = 0;
            if (dataOut.size() != expectedData.size())
            {
                printf("Size mismatch in dataOut! Expected %4zu Actual %4zu\n", expectedData.size(), dataOut.size());
                dataFail++;
            }

            for (size_t i = 0; i < expectedData.size(); i++)
            {
                dataTotal++;
                if (dataOut.size() >= i)
                {
                    if (expectedData[i] == dataOut[i])
                    {
                        dataMatch++;
                    }
                    else
                    {
                        printf("Mismatch in dataOut at index %4zu! Expected %02X Actual %02X\n", i, expectedData[i], dataOut[i]);
                        dataFail++;
                    }
                }
            }
            printf("Matched %4u of %4u failed %4u, test %s\n", dataMatch, dataTotal, dataFail, dataFail == 0 ? "Passed" : "Failed");
            unitTestPassed += dataFail == 0 ? 1 : 0;
        }
        unitTestIndex++;
    }
    if (testResults)
    {
        printf("\nPassed %4u of %4u tests! Unittest complete!\n", unitTestPassed, unitTestIndex);
    }
}

uint8_t SelectionKernel(uint8_t X, uint8_t A, uint8_t B, uint8_t C, uint16_t D, uint8_t E, uint8_t F, uint8_t Y1, uint8_t Y2);

uint8_t GetBit(uint32_t source, uint8_t index, uint8_t outIndex)
{
    uint8_t returnVal = (source >> index) & 1;
    return returnVal << outIndex;
}
void TestHop()
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
    uint32_t address = 0x6587CBA9;
    // A9 -> 10101 -> 0x15
    // 96 -> 0010 -> 0x02
    // F25 -> 10011 -> 0x13
    // 6EF -> 110111011 -> 0x1BB
    // EF25 -> 1110100 -> 0x74
    uint8_t koffset = 24;
    uint8_t knudge = 0;
    for(uint32_t clk = 0; clk < 0x00040000; clk += 0x1000)
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

        if(clk + 0x1000 >= 0x0040000)
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

