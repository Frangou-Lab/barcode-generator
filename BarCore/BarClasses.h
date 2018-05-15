/*
* Copyright 2018 Frangou Lab
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#ifdef __APPLE__
typedef int32_t long_t;
#else
typedef long long_t;
#endif

typedef unsigned char t_code;
#define CODE_SIZE 4
#define MAX_MUTATIONS 4
#define MAX_COUNT 16383

#ifdef _WIN64

// Use memory paging to reduce node size
//#define PAGED_POINTERS
#endif

class BarNodeManager;
extern BarNodeManager *g_nodeManager;

struct SearchResult
{
    int position;
    int length;
};
SearchResult findColumn(const char *string, int n, char separator);


class BarCode
{
    friend bool recurDumpCode(BarCode *code, int pos);

public:
    BarCode(int len, int totalFiles, int totalMPs);
    ~BarCode();

    static int detectCodeLen(char *str, int colNumber, char separator);

    int loadFromStringMP(const char *str, int colNumber, char separator, int mpLen, int mpCount, int *mpMask, char **mpData);

    bool loadFromString(const char *str, int colNumber, int startPos, int descColNumber, char separator);
    bool loadFromString(const char *str, int colNumber, int descColNumber, char separator);
    void dump(char *to);
    t_code symbol(int pos);

    void initMutations();
    bool mutate(int mutationLevel); // Returns false if no more mutations available

    bool mutate() {return mutate(0);}
    bool mutate2() {return mutate(1);}

    void setNTo(t_code code);
    int len() {return m_len;}

    long getExactCount(int n, int mp) {return m_countExact[m_totalFiles*mp+n];}
    long get1ErrorCount(int n, int mp) {return m_count1Error[m_totalFiles*mp+n];}
    long get2ErrorCount(int n, int mp) {return m_count2Error[m_totalFiles*mp+n];}
    long getNCount(int n, int mp) {return m_countN[m_totalFiles*mp+n];}
    long getIndelCount(int n, int mp) {return m_countIndel[m_totalFiles*mp+n];}
    long getInsCount(int n, int mp) {return m_countIns[m_totalFiles*mp+n];}

    void incExactCount(int n, int mp, int increment);
    void inc1ErrorCount(int n, int mp, int increment);
    void inc2ErrorCount(int n, int mp, int increment);
    void incNCount(int n, int mp, int increment);
    void incIndelCount(int n, int mp, int increment);
    void incInsCount(int n, int mp, int increment);

    bool hasZeroCounts();
    long getTotalCount(int n, int mp);

    // Insertion & indel support
    // Returns <0 if indel, >0 if insertion and 0 if no
    int hasInsertionOrIndel();
    void prepare(); // Initializes internals for next call
    bool nextSuitable(); // Returns false if no one

    void resetToOriginal()
    {
        if (m_originalCode)
            memcpy(m_code, m_originalCode, sizeof(t_code)*m_len);
    }
    void saveOriginal()
    {
        if (!m_originalCode)
            m_originalCode = (t_code *)malloc(sizeof(t_code)*m_len);
        memcpy(m_originalCode, m_code, sizeof(t_code)*m_len);
    }

    void generateRandom(); // Generates random code in ORIGINAL
    int hammingDistance(BarCode *code);

private:
    t_code *m_code, *m_originalCode;
    int m_len, m_totalFiles, m_totalMPs;
    int m_mutation[MAX_MUTATIONS];
    //int m_mutation2;
    volatile long_t *m_countExact;
    volatile long_t *m_count1Error, *m_count2Error;
    volatile long_t *m_countN, *m_countIndel, *m_countIns;

    int m_realLen;
    int m_curIPos;
    int m_curIndelChar;

public:
    int m_NPosition;
    char *m_strCode;
    char *m_strDesc;
//	int m_indexInArray;
};

class BarNode;

class SimpleBarNode;
class PagedPointerBarNode;

class BarNodeManager
{
public:
    BarNodeManager();
    virtual ~BarNodeManager();

    virtual BarNode* getChildNode(BarNode* parent, int childIndex) = 0;
    virtual BarNode* newChildNode(BarNode* parent, int childIndex) = 0;
    virtual BarCode* getBarCode(BarNode* node) = 0;
    virtual void setBarCode(BarNode* node, BarCode* barCode) = 0;
    virtual int getErrors(BarNode* node) = 0;
    virtual void setErrors(BarNode* node, int errors) = 0;	
    BarNode* newRoot() { return newChildNode(nullptr, 0); }
    virtual unsigned int getPageSize() = 0;

protected:
    void addPage();
    std::vector<uint8_t*> m_pages;
    uint32_t m_pos;
};

#pragma pack (push, 4)
enum NodeKind {
    SimpleBarNodeKind,
    PagedPointerBarNodeKind
};
class BarNode
{
public:
    inline BarNode *getChild(int childIndex)  { return g_nodeManager->getChildNode(this, childIndex); }
    inline BarNode *newChild(int i) { return g_nodeManager->newChildNode(this, i); }
    inline BarCode* getBarCode() { return g_nodeManager->getBarCode(this); }
    inline void setBarCode(BarCode* barCode) { return g_nodeManager->setBarCode(this, barCode); }
    inline int getErrors() { return g_nodeManager->getErrors(this); }
    inline void setErrors(int errors) { return g_nodeManager->setErrors(this, errors); }
};

class SimpleBarNode : public BarNode
{
public:
    union
    {
        SimpleBarNode *m_childs[CODE_SIZE];
        struct
        {
            BarCode *m_code;
            int m_errors;
        };
    };
};

class PagedPointerBarNode : public BarNode
{
public:
    union
    {
        struct
        {
            uint32_t m_offsets[CODE_SIZE];
            uint8_t m_pages[CODE_SIZE];
        };
        struct
        {
            BarCode *m_code;
            int m_errors;
        };
    };
};

class SimpleBarNodeManager : public BarNodeManager
{
    SimpleBarNode* typifiedNode(BarNode* node) {return ((SimpleBarNode*)node); }	
public:
    BarNode* getChildNode(BarNode* parent, int childIndex);
    BarNode* newChildNode(BarNode* parent, int childIndex);
    unsigned int getPageSize();
    BarCode* getBarCode(BarNode* node) { return typifiedNode(node)->m_code; };
    void setBarCode(BarNode* node, BarCode* barCode) { typifiedNode(node)->m_code = barCode; };
    int getErrors(BarNode* node) { return typifiedNode(node)->m_errors; };
    void setErrors(BarNode* node, int errors) { typifiedNode(node)->m_errors = errors; };	
};

class PagedPointerBarNodeManager : public BarNodeManager
{
protected:
    PagedPointerBarNode* typifiedNode(BarNode* node) {return ((PagedPointerBarNode*)node); }	
    
public:
    BarNode* getChildNode(BarNode* parent, int childIndex);
    BarNode* newChildNode(BarNode* parent, int childIndex);
    unsigned int getPageSize();
    BarCode* getBarCode(BarNode* node) { return typifiedNode(node)->m_code; };
    void setBarCode(BarNode* node, BarCode* barCode) { typifiedNode(node)->m_code = barCode; };
    int getErrors(BarNode* node) { return typifiedNode(node)->m_errors; };
    void setErrors(BarNode* node, int errors) { typifiedNode(node)->m_errors = errors; };
    
};
#pragma pack (pop)

class BarArray
{
public:
    BarArray();
    ~BarArray();

    int put(BarCode *code);
    BarCode *get(int index);
    int count();
private:
    BarCode **m_data;
    int m_bufLen;
    int m_realLen;
};

