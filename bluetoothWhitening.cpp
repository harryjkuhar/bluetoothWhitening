#include <stdio.h>
#include <stdint.h>
#include "BluetoothWhitening.h"
#include "LinearFeedbackShiftRegister.h"

#include <vector>

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
        lsfr.ClearDataOut(0);
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

        lsfr.ClearDataOut(0);
        lsfr.Shift(static_cast<uint32_t>(dataIn.size() * 8), dataIn);
        
        crcVal = (uint16_t)lsfr.GetState();
//        crcVal = lsfr.GetDataOut(0)[dataIn.size()] | lsfr.GetDataOut(0)[dataIn.size() + 1] << 8;
    }
//    FEDC BA98 7654 3210
//    1011 0110 0100 1011
//    1101 0010 0110 1101
//    D    2    6    D
private:

    LinearFeedbackShiftRegister lsfr;
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

int main(int argc, const char* argv[])
{
    std::vector<uint8_t> testData;
    std::vector<uint8_t> dataOut;
    uint8_t btClock = 0;
    uint32_t temp = 0;
    bool btClockParsed = false;
    bool forcebinaryFile = false;
    bool crcMode = false;

    for (int i = 1; i < argc; i++)
    {
        char* endPtr = nullptr;
        if (strcmp(argv[i], "--b") == 0)
        {
            forcebinaryFile = true;
        }
        else if (strcmp(argv[i], "--c") == 0)
        {
            crcMode = true;
        }
        else if (btClockParsed == false)
        {
            temp = strtol(argv[i], &endPtr, 16);
            if (endPtr != argv[i])
            {
                if (temp >> 7 != 0)
                {
                    printf("Warning Bluetooth clock has too many bits! Only using lowest 7 bits!\n");
                }
                btClockParsed = true;
                btClock = temp;
            }
        }
        else
        {
            temp = strtol(argv[i], &endPtr, 16);
            if (endPtr != argv[i])
            {
                testData.push_back(temp & 0xFF);
            }
            else
            {
                size_t bytesRead = 0;
                if (forcebinaryFile == false)
                {
                    FILE* dataFile = nullptr;
                    fopen_s(&dataFile, argv[i], "rt");
                    if (dataFile != nullptr)
                    {
                        int conversions = 1;

                        while (conversions > 0 && feof(dataFile) == false)
                        {
                            conversions = fscanf_s(dataFile, "%02X", &temp);
                            if (conversions > 0)
                            {
                                testData.push_back(temp & 0xFF);
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
                    fopen_s(&dataFile, argv[i], "rt");
                    if (dataFile != nullptr)
                    {
                        fseek(dataFile, 0L, SEEK_END);
                        size_t length = ftell(dataFile);
                        fseek(dataFile, 0L, SEEK_SET);
                        size_t startIndex = testData.size();
                        testData.resize(startIndex + length);
                        fread(&testData[startIndex], 1, length, dataFile);
                    }
                }
                
            }

        }
    }

    if (btClockParsed == false)
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
        uint16_t crcVal;
        BluetoothCrc crcGen(btClock);
        crcGen.CalcCrc(testData, crcVal);
        printf("uap %02X crc %04X\n", btClock, crcVal);
    }
    else
    {
        //60 10 D0 00 C1 9E 81 3F AB 74 72 97 86 5D 64 0C 01 2A C2 CB E7 09
        BluetoothWhitening whitening(btClock);
        whitening.WhitenData(testData, dataOut);

        for (size_t i = 0; i < dataOut.size(); i++)
        {
            printf("%02X ", dataOut[i]);
        }
        printf("\n");
    }
}

