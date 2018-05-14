#include "BarCore.h"

#include <Windows.h>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

t_code char2t_code(char c)
{
    char *ret = strchr(g_alphabet, (int)c);
    if (!ret)
        return (t_code)0x7f;
    return (t_code)(ret - g_alphabet);
}

SearchResult findColumn(const char *string, int n, char separator)
{
    SearchResult ret;
    ret.position = -1;
    int curN = 0;
    const char *curPos = string;
    do
    {
        const char *newPos = strchr(curPos, separator);
        if (newPos == NULL) // last
        {
            int q = strlen(curPos);
            newPos = curPos + q;
        }
        if (curN == n) // this one
        {
            ret.position = (int)(curPos - string);
            ret.length = (int)(newPos - curPos);
            break;
        }
        if (!*newPos) // EOS
            break;
        curPos = newPos + 1;
        ++curN;
    } while (1);
    return ret;
}

BarCode::BarCode(int len, int totalFiles, int totalMPs)
{
    m_len = len;
    m_strCode = (char *)malloc(sizeof(char)*(m_len+2));
    m_strCode[len] = 0;
    m_code = (t_code *)malloc(sizeof(t_code)*(m_len+1));
    m_strDesc = NULL;
    m_originalCode = 0;
    m_mutation[0] = 0;

    m_totalFiles = totalFiles;
    m_totalMPs = totalMPs;
    if (m_totalFiles*m_totalMPs)
    {
        m_countExact = (long_t *)malloc(m_totalFiles*m_totalMPs*sizeof(long_t));
        memset((void *)m_countExact, 0, m_totalFiles*m_totalMPs*sizeof(long_t));
        m_count1Error = (long_t *)malloc(m_totalFiles*m_totalMPs*sizeof(long_t));
        memset((void *)m_count1Error, 0, m_totalFiles*m_totalMPs*sizeof(long_t));
        m_count2Error = (long_t *)malloc(m_totalFiles*m_totalMPs*sizeof(long_t));
        memset((void *)m_count2Error, 0, m_totalFiles*m_totalMPs*sizeof(long_t));
        m_countN = (long_t *)malloc(m_totalFiles*m_totalMPs*sizeof(long_t));
        memset((void *)m_countN, 0, m_totalFiles*m_totalMPs*sizeof(long_t));
        m_countIns = (long_t *)malloc(m_totalFiles*m_totalMPs*sizeof(long_t));
        memset((void *)m_countIns, 0, m_totalFiles*m_totalMPs*sizeof(long_t));
        m_countIndel = (long_t *)malloc(m_totalFiles*m_totalMPs*sizeof(long_t));
        memset((void *)m_countIndel, 0, m_totalFiles*m_totalMPs*sizeof(long_t));
    }
}

BarCode::~BarCode()
{
    free(m_code);
    free(m_strCode);

    if (m_originalCode)
        free(m_originalCode);
    if (m_strDesc)
        free(m_strDesc);

    if (m_totalFiles*m_totalMPs)
    {
        free ((void *)m_countExact);
        free ((void *)m_count1Error);
        free ((void *)m_count2Error);
        free ((void *)m_countN);
        free ((void *)m_countIns);
        free ((void *)m_countIndel);
    }
}

void BarCode::setNTo(t_code code)
{
#ifdef DEBUG
    assert(m_NPosition >= 0);
#endif
    m_code[m_NPosition] = code;
}

t_code BarCode::symbol(int pos)
{
    return m_code[pos];
}

void BarCode::dump(char *tmp)
{
    for (int i = 0; i < m_len; i++)
        tmp[i] = g_alphabet[m_code[i]];
    tmp[m_len] = 0;
}

int BarCode::loadFromStringMP(const char *str, int colNumber, char separator, int mpLen, int mpCount, int *mpMask, char **mpData)
{
    m_NPosition = -1;

    SearchResult r = findColumn(str, colNumber, separator);
    if (r.position < 0)
        return -1;

    // No insertions or indel handling here
    if (r.length != m_len + mpLen)
        return -1;

    char *myMp = new char[mpLen+1];
    myMp[mpLen] = 0;
    m_realLen = m_len;
    // Copy chars and fill mp
    int j = 0;
    int mpIdx = 0;
    for (int i = 0; i < r.length; i++)
    {
        if (mpMask[i]) // Add to mp
            myMp[mpIdx++] = str[r.position+i];
        else // regular
            m_strCode[j++] = str[r.position+i];
    }

    // Find in mps
    int resMP = -1;
    for (int i = 0; i < mpCount; i++)
        if (strcmp(myMp, mpData[i]) == 0) // Hit!
        {
            resMP = i;
            break;
        }

    delete[] myMp;

    if (resMP < 0) // Not found in MPs
        return -1;

    for (int i = 0; i < m_realLen; i++)
        if (m_strCode[i] == 'N') // Special case
        {
            if (m_NPosition >= 0) // It's double N, skip
                return -1;
            else
                m_NPosition = i;
            m_code[i] = CODE_SIZE;
        }
        else
            if ((m_code[i] = char2t_code(m_strCode[i])) >= CODE_SIZE)
                return -1;

    // No description here 
    return resMP;
}

int BarCode::detectCodeLen(char *str, int colNumber, char separator)
{
    SearchResult r = findColumn(str, colNumber, separator);
    if (r.position < 0)
        return false;
    return r.length;
}

bool BarCode::loadFromString(const char *str, int colNumber, int descColNumber, char separator)
{
    return loadFromString(str, colNumber, -1, descColNumber, separator);
}

bool BarCode::loadFromString(const char *str, int colNumber, int startPos, int descColNumber, char separator)
{
    m_NPosition = -1;

    SearchResult r = findColumn(str, colNumber, separator);
    if (r.position < 0)
        return false;


    if (startPos <= 0) // No position, detect old way
    {
        if (r.length != m_len)
        {
            if (abs(r.length - m_len) != 1) // Bad code
                return false; 
        }
        m_realLen = r.length;
    }
    else // Get only exact m_len positions, if able
    {
        r.position += startPos-1;
        r.length -= startPos-1;

        if (r.length < m_len) // To few data
            return false;
        m_realLen = m_len;
    }

    memcpy(m_strCode, str+r.position, m_realLen*sizeof(char));
    for (int i = 0; i < m_realLen; i++)
        if (m_strCode[i] == 'N') // Special case
        {
            if (m_NPosition >= 0 || m_realLen != m_len) // It's double N or insertion/indel with N, skip
                return false;
            else
                m_NPosition = i;
            m_code[i] = CODE_SIZE;
        }
        else
            if ((m_code[i] = char2t_code(m_strCode[i])) >= CODE_SIZE)
                return false;

    // Description
    if (descColNumber >= 0)
    {
        // Now all is desc
        m_strDesc = (char *)malloc((strlen(str)+1)*sizeof(char));
        strcpy_s(m_strDesc, strlen(str)+1, str);
        m_strDesc[strlen(str)] = 0;
    }

    return true;
}


void BarCode::initMutations()
{
    memset(m_mutation, 0, MAX_MUTATIONS*sizeof(int));
    resetToOriginal();
}

bool BarCode::mutate(int mutationLevel)
{
    _ASSERTE(mutationLevel < MAX_MUTATIONS);

    if (mutationLevel+1 < MAX_MUTATIONS) // Init next
        m_mutation[mutationLevel+1] = m_mutation[mutationLevel]+CODE_SIZE;
    if (mutationLevel == 0 && m_mutation[0] == 0)
        saveOriginal();

    int curMutation = m_mutation[mutationLevel]%(CODE_SIZE-1);
    int mutation_symbol = m_mutation[mutationLevel]/(CODE_SIZE-1);

    int mutation_symbol1 = 0;
    if (mutationLevel > 0)
        mutation_symbol1 = (m_mutation[mutationLevel-1])/(CODE_SIZE-1);

    if (curMutation == 0) // New mutation, put back previous
    {
        if (mutation_symbol > mutation_symbol1)
            m_code[mutation_symbol-1] = m_originalCode[mutation_symbol-1];
    }

    if (mutation_symbol >= m_len)
        return false;

    // Mutate to next symbol, skipping original
    if (curMutation >= m_originalCode[mutation_symbol])
        ++curMutation;
    m_code[mutation_symbol] = curMutation;
    ++m_mutation[mutationLevel];

    if (/* DISABLES CODE */ (false))
    {
        char buf[1024];
        dump(buf);
        strcat_s(buf, 1024, "\n");
#ifndef __APPLE__
        OutputDebugStringA(buf);
#endif
    }

    return true;
}

void BarCode::generateRandom()
{
    for (int i = 0; i < m_len; i++)
        m_originalCode[i] = rand()%CODE_SIZE;
}

void BarCode::incIndelCount(int n, int mp, int increment)
{
    InterlockedExchangeAdd(m_countIndel+m_totalFiles*mp+n, increment);
}
void BarCode::incInsCount(int n, int mp, int increment)
{
    InterlockedExchangeAdd(m_countIns+m_totalFiles*mp+n, increment);
}

void BarCode::incExactCount(int n, int mp, int increment)
{
    _ASSERTE(mp*m_totalFiles+n < m_totalFiles*m_totalMPs);
    InterlockedExchangeAdd(m_countExact+m_totalFiles*mp+n, increment);
}
void BarCode::inc1ErrorCount(int n, int mp, int increment)
{
    InterlockedExchangeAdd(m_count1Error+m_totalFiles*mp+n, increment);
}
void BarCode::inc2ErrorCount(int n, int mp, int increment)
{
    InterlockedExchangeAdd(m_count2Error+m_totalFiles*mp+n, increment);
}
void BarCode::incNCount(int n, int mp, int increment)
{
    InterlockedExchangeAdd(m_countN+m_totalFiles*mp+n, increment);
}

long BarCode::getTotalCount(int i, int mp)
{
    return m_countExact[m_totalFiles*mp+i] + m_count1Error[m_totalFiles*mp+i] + m_count2Error[m_totalFiles*mp+i] + m_countN[m_totalFiles*mp+i] + m_countIndel[m_totalFiles*mp+i] + m_countIns[m_totalFiles*mp+i];
}

bool BarCode::hasZeroCounts()
{
    for (int i = 0; i < m_totalFiles; i++)
        for (int j = 0; j < m_totalMPs; j++)
            if (getTotalCount(i, j) > 0)
                return false;
    return true;
}

int BarCode::hasInsertionOrIndel() 
{
    if (m_len == m_realLen) // Strict
        return 0;
    if (m_realLen > m_len) // Ins
        return 1;
    return -1;
}

void BarCode::prepare()
{
    if (!m_originalCode)
        m_originalCode = (t_code *)malloc(sizeof(t_code)*(m_len+1));
    memcpy(m_originalCode, m_code, sizeof(t_code)*m_realLen);
    m_curIndelChar = 0;
    m_curIPos = 0;
}
bool BarCode::nextSuitable() // Returns false if no one
{
    if (m_realLen > m_len) // insertion, delete some and check
    {
        if (m_curIPos == m_realLen)
            return false;
        memcpy(m_code, m_originalCode, sizeof(t_code)*m_realLen);
        memmove(m_code+m_curIPos, m_code+m_curIPos+1, sizeof(t_code)*(m_realLen-m_curIPos-1));
        ++m_curIPos;
        return true;
    }
    // Well, it's indel, insert some chars
    if (m_curIPos == m_len)
        return false;
    if (m_curIndelChar == 0) // New char
    {
        memcpy(m_code, m_originalCode, sizeof(t_code)*m_realLen);
        memmove(m_code+m_curIPos+1, m_code+m_curIPos, sizeof(t_code)*(m_len - m_curIPos - 1));
    }
    m_code[m_curIPos] = m_curIndelChar;
    if (++m_curIndelChar >= CODE_SIZE)
    {
        m_curIndelChar = 0;
        ++m_curIPos;
    }
    return true;
}

int BarCode::hammingDistance(BarCode *code)
{
    int dist = 0;
    for (int i = 0; i < m_len; i++)
        if (m_code[i] != code->m_code[i])
            ++dist;
    return dist;
}

#define START_BUF_SIZE	8192

BarArray::BarArray()
{
    m_realLen = 0;
    m_bufLen = START_BUF_SIZE;
    m_data = (BarCode **)malloc(m_bufLen*sizeof(BarCode **));
}
BarArray::~BarArray()
{
    for (int i = 0; i < m_realLen; i++)
        delete m_data[i];
    free(m_data);
}

int BarArray::put(BarCode *code)
{
    if (m_realLen >= m_bufLen) // Realloc
    {
        m_bufLen += START_BUF_SIZE;
        m_data = (BarCode **)realloc(m_data, m_bufLen*sizeof(BarCode **));
    }
    m_data[m_realLen] = code;
    return m_realLen++;
}

BarCode *BarArray::get(int index)
{
    _ASSERTE(index < m_realLen);
    return m_data[index];
}
int BarArray::count()
{
    return m_realLen;
}

BarNodeManager::BarNodeManager()
{
}

BarNodeManager::~BarNodeManager()
{
    for (uint8_t *page : m_pages)
        free(page);
}

void BarNodeManager::addPage()
{
    m_pos = 0;
    uint8_t *page = (uint8_t *)malloc(getPageSize());
    if (page == nullptr) {
        std::cerr << "panic! out of memory\n";
        exit(1);
    }

    m_pages.push_back(page);
}

unsigned int SimpleBarNodeManager::getPageSize()
{
    return 1024*1024*sizeof(SimpleBarNode);
}

unsigned int PagedPointerBarNodeManager::getPageSize()
{
    return 1024u*1024u*1024u*2u;
}

BarNode *SimpleBarNodeManager::newChildNode(BarNode* parent, int childIndex)
{
    if (m_pos+sizeof(SimpleBarNode) >= getPageSize() || m_pages.empty())
        addPage();

    SimpleBarNode *node = (SimpleBarNode *)(m_pages.back() + m_pos);
    // Init node
    memset(node->m_childs, 0, CODE_SIZE*sizeof(SimpleBarNode *));	
    m_pos += sizeof(SimpleBarNode);	
    if (parent != NULL)
    {
        SimpleBarNode* typifiedParent = (SimpleBarNode*) parent;
        typifiedParent->m_childs[childIndex] = node;
    }
    return node;
}

BarNode *PagedPointerBarNodeManager::newChildNode(BarNode *parent, int childIndex)
{
    if (m_pos + sizeof(PagedPointerBarNode) >= getPageSize() || m_pages.empty())
        addPage();

    if (parent != NULL)
    {
        PagedPointerBarNode* typifiedParent = (PagedPointerBarNode*) parent;
        typifiedParent->m_pages[childIndex] = static_cast<int>(m_pages.size()) - 1;
        typifiedParent->m_offsets[childIndex] = m_pos;
    }
    PagedPointerBarNode *node = (PagedPointerBarNode*)(m_pages.back() + m_pos);
    memset(node->m_offsets, 0, CODE_SIZE*sizeof(uint32_t) + CODE_SIZE);
    m_pos += sizeof(PagedPointerBarNode);
    return node;
}

BarNode *SimpleBarNodeManager::getChildNode(BarNode *parent, int childIndex)
{
    SimpleBarNode *typifiedParent = (SimpleBarNode*)parent;
    return typifiedParent->m_childs[childIndex];
}

BarNode *PagedPointerBarNodeManager::getChildNode(BarNode *parent, int childIndex)
{
    PagedPointerBarNode* typifiedParent = (PagedPointerBarNode*)parent;
    int page = typifiedParent->m_pages[childIndex];
    int offset = typifiedParent->m_offsets[childIndex];
    if (offset == 0 && page == 0) // We assume that root always allocated in 0,0 and newer be asked via this method
        return NULL;
    return (BarNode *)(m_pages[page]+offset);
}
