#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <Windows.h>
#include <TlHelp32.h>
#include <map>

struct MemRegion
{
    uintptr_t baseAddress;
    SIZE_T regionSize;
    DWORD state;
    DWORD protect;
    DWORD type;
};

class MemUtility
{
public:
    MemUtility(const char* processName, size_t bufferCapacity = 32);
    ~MemUtility();

    void openProcess(DWORD accessRights = PROCESS_ALL_ACCESS);
    void closeProcess();

    void readMemory(DWORD source, size_t size);
    void writeMemory(const void* source, uintptr_t destination, size_t size);

    bool isAddressValid(DWORD address, size_t size);
    std::map<uintptr_t, MemRegion> getMemoryRegions();

    void autoFindMemAndWrite(int value);

    template <typename T>
    inline T getBuffer()
    {
        return *reinterpret_cast<T*>(this->buffer.data());
    }

private:
    std::string processName;
    DWORD processID;
    HANDLE processHandle;
    std::vector<byte> buffer;
    size_t bufferCapacity;

    void findProcessID();
};