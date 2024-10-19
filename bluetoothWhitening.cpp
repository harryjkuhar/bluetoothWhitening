#include <stdio.h>
#include <stdint.h>
#include "BluetoothWhitening.h"
#include "LinearFeedbackShiftRegister.h"

#include <vector>
#include <string>

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
        size_t dataIndex = 0;

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
        size_t dataIndex = 0;

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
bool testResults = false;
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
    testResults = false;

    for (int i = 1; i < args.size(); i++)
    {
        char* endPtr = nullptr;

        if (args[i] == "--u")
        {
            unitTestIndex = 0;
            return;
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
            for (int i = 0; i < unitTests[unitTestIndex].size(); i++)
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

        if (crcMode)
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
 
