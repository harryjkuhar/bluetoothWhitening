#pragma once
#include <stdint.h>
#include <vector>

struct GeneratorState
{
    GeneratorState()
    {
        m_workByte = 0;
        m_bitIndex = 0;
    }
    void clear()
    {
        m_dataOut.clear();
        m_workByte = 0;
        m_bitIndex = 0;
    }
    std::vector<uint8_t> m_dataOut;
    uint8_t m_workByte;
    uint32_t m_bitIndex;
};

class LinearFeedbackShiftRegister
{
public:
    LinearFeedbackShiftRegister(uint32_t registerCount, uint32_t initState)
    {
        m_BitShiftValue = 0x80;
        m_rightShift = true;
        m_registerCount = registerCount;
        m_initState = initState;
        
        if (m_registerCount > sizeof(m_initState) * 8)
        {
            m_registerCount = sizeof(m_initState) * 8;
        }
        
        m_registerInputs.resize(m_registerCount);
        m_nextStates.resize(m_registerCount);

        for (uint32_t i = 0; i < m_registerCount; i++)
        {
            if (i + 1 < m_registerCount)
            {
                m_registerInputs[i + 1].push_back(i);
            }
            m_states.push_back(m_initState & 1);
            m_initState >>= 1;
        }
    }

    void AddGaloisPoly(uint32_t poly)
    {
        // final register must be feedback for Galois, no need to check that poly bit
        uint32_t polyBitCount = m_registerCount;
        for (uint32_t i = 0; i < polyBitCount; i++)
        {
            if (((poly >> i) & 1) == 1)
            {
                m_registerInputs[i].push_back(m_registerCount-1);
            }
        }
    }

    void AddGeneratorPoly(uint32_t poly)
    {
        uint32_t polyBitCount = m_registerCount + 1;
        m_GeneratorInputs.emplace_back();
        m_genOut.emplace_back();
        std::vector<uint32_t>& genIn = m_GeneratorInputs[m_GeneratorInputs.size()-1];
        
        for (uint32_t i = 0; i < polyBitCount; i++)
        {
            if (((poly >> i) & 1) == 1)
            {
                genIn.push_back(i);
            }
        }
    }

    void Shift(uint32_t bitCount = 1)
    {
        for (uint32_t i = 0; i < bitCount; i++)
        {
            for (uint32_t regIndex = 0; regIndex < m_registerCount; regIndex++)
            {
                m_nextStates[regIndex] = XorInputs(regIndex);
            }
            for (uint32_t genIndex = 0; genIndex < m_GeneratorInputs.size(); genIndex++)
            {
                std::vector<uint32_t>& genIn = m_GeneratorInputs[genIndex];

                for (uint32_t regIndex = 0; regIndex < genIn.size(); regIndex++)
                {
                    m_genOut[genIndex].m_workByte ^= m_states[genIn[regIndex]] ? m_BitShiftValue : 0;
                }
                
                if (m_genOut[genIndex].m_bitIndex < 7)
                {
                    m_genOut[genIndex].m_bitIndex++;
                    if (m_rightShift)
                    {
                        m_genOut[genIndex].m_workByte >>= 1;
                    }
                    else
                    {
                        m_genOut[genIndex].m_workByte <<= 1;
                    }
                }
                else
                {
                    m_genOut[genIndex].m_dataOut.push_back(m_genOut[genIndex].m_workByte);
                    m_genOut[genIndex].m_bitIndex = 0;
                    m_genOut[genIndex].m_workByte = 0;
                }

            }
            for (uint32_t regIndex = 0; regIndex < m_registerCount; regIndex++)
            {
                m_states[regIndex] = m_nextStates[regIndex];
            }
        }
    }

    uint32_t XorInputs(uint32_t regIndex)
    {
        uint32_t output = 0;
        for(uint32_t i = 0; i < m_registerInputs[regIndex].size(); i++)
        {
            uint32_t stateIndex = m_registerInputs[regIndex][i];
            output ^= m_states[stateIndex];
        }
        return output;
    }

    std::vector<uint8_t> GetDataOut(uint32_t index) 
    { 
        if (m_genOut[index].m_bitIndex != 0)
        {
            m_genOut[index].m_workByte >>= 7 - m_genOut[index].m_bitIndex;
            m_genOut[index].m_dataOut.push_back(m_genOut[index].m_workByte);
            m_genOut[index].m_bitIndex = 0;
            m_genOut[index].m_workByte = 0;
        }
        return m_genOut[index].m_dataOut; 
    }
    void ClearDataOut(uint32_t index)
    {
        m_genOut[index].clear();
    }
    size_t GetGeneratorCount() { return m_genOut.size(); }

    uint8_t GetBitShiftValue() { return m_BitShiftValue;}
    void SetBitShiftValue(uint8_t val) { m_BitShiftValue = val; }

private:
    uint8_t m_BitShiftValue;
    bool m_rightShift;
    uint32_t m_registerCount;
    uint32_t m_initState;
//    uint32_t m_feedBackPoly;
//    std::vector<uint32_t> m_generatorPolys;
    std::vector< std::vector <uint32_t>> m_registerInputs;
    std::vector< std::vector <uint32_t>> m_GeneratorInputs;
    std::vector<uint32_t> m_states;
    std::vector<uint32_t> m_nextStates;
    std::vector<GeneratorState> m_genOut;
};
