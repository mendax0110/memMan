#include "../include/MemUtility.h"

MemUtility::MemUtility(const char* processName, size_t bufferCapacity)
    : processName(processName), processID(0), processHandle(nullptr), bufferCapacity(bufferCapacity)
{
    this->buffer.resize(bufferCapacity);
}

MemUtility::~MemUtility()
{
    closeProcess();
}

/**
 * @brief This method will search and find the ID of the Process
 */
void MemUtility::findProcessID()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        throw std::runtime_error("Failed to create process snapshot");
    }

    PROCESSENTRY32 procInfo;
    procInfo.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(snapshot, &procInfo))
    {
        CloseHandle(snapshot);
        throw std::runtime_error("Failed to get process ID");
    }

    do
    {
        if (_stricmp(this->processName.c_str(), procInfo.szExeFile) == 0)
        {
            this->processID = procInfo.th32ProcessID;
            CloseHandle(snapshot);
            return;
        }
    } while (Process32Next(snapshot, &procInfo));

    CloseHandle(snapshot);
    throw std::runtime_error("Failed to find process ID");
}

/**
 * @brief This method will open the process
 * @param accessRights These are the access rights of the process
 */
void MemUtility::openProcess(DWORD accessRights)
{
    if (this->processID == 0)
    {
        findProcessID();
    }

    this->processHandle = OpenProcess(accessRights, FALSE, this->processID);
    if (this->processHandle == nullptr)
    {
        throw std::runtime_error("Failed to open process");
    }
}

/**
 * @brief This is method will close the process
 */
void MemUtility::closeProcess()
{
    if (this->processHandle != nullptr)
    {
        CloseHandle(this->processHandle);
        this->processHandle = nullptr;
    }
}

/**
 * @brief This method is in charge of writing to the memory
 * @param source -> This is the source of the memroy
 * @param destination -> This is the destination of the mem address
 * @param size -> This is the size of the memroy
 */
void MemUtility::writeMemory(const void* source, uintptr_t destination, size_t size)
{
    if (this->processHandle == nullptr)
    {
        throw std::runtime_error("Process is not open");
    }

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(this->processHandle, reinterpret_cast<LPCVOID>(destination), &mbi, sizeof(mbi)) == 0)
    {
        DWORD error = GetLastError();
        throw std::runtime_error("VirtualQueryEx failed, error code: " + std::to_string(error));
    }

    if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS)
    {
        throw std::runtime_error("Memory region is not in a committed state or has no access protection.");
    }

    DWORD oldProtect;
    if (!VirtualProtectEx(this->processHandle, reinterpret_cast<LPVOID>(destination), size, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        DWORD error = GetLastError();
        throw std::runtime_error("Failed to change memory protection, error code: " + std::to_string(error));
    }

    SIZE_T bytesWritten = 0;
    if (!WriteProcessMemory(this->processHandle, reinterpret_cast<LPVOID>(destination), source, size, &bytesWritten))
    {
        DWORD error = GetLastError();
        throw std::runtime_error("Failed to write memory, error code: " + std::to_string(error));
    }

    if (bytesWritten != size)
    {
        throw std::runtime_error("Incomplete write: " + std::to_string(bytesWritten) + " bytes written, expected " + std::to_string(size));
    }

    if (!VirtualProtectEx(this->processHandle, reinterpret_cast<LPVOID>(destination), size, oldProtect, &oldProtect))
    {
        DWORD error = GetLastError();
        throw std::runtime_error("Failed to restore memory protection, error code: " + std::to_string(error));
    }
}

/**
 * @brief This method will read the memory
 * @param source -> This is the source of the memroy
 * @param size -> This is the size of the memroy
 */
void MemUtility::readMemory(DWORD source, size_t size)
{
    if (this->processHandle == nullptr)
    {
        throw std::runtime_error("Process is not open");
    }

    if (this->buffer.size() < size)
    {
        this->buffer.resize(size);
    }

    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(this->processHandle, reinterpret_cast<LPCVOID>(source), this->buffer.data(), size, &bytesRead))
    {
        DWORD error = GetLastError();
        throw std::runtime_error("Failed to read memory, error code: " + std::to_string(error));
    }
}

/**
 * @brief This method will extract all the memory regions
 * @return std::map<uintptr_t, MemRegion> -> This is the map with the memoryregion infos
 */
std::map<uintptr_t, MemRegion> MemUtility::getMemoryRegions()
{
    if (this->processHandle == nullptr)
    {
        throw std::runtime_error("Process is not open");
    }

    std::map<uintptr_t, MemRegion> memoryRegions;
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t address = 0;

    while (VirtualQueryEx(this->processHandle, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)))
    {
        MemRegion region;
        region.baseAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        region.regionSize = mbi.RegionSize;
        region.state = mbi.State;
        region.protect = mbi.Protect;
        region.type = mbi.Type;

        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & PAGE_READONLY ||
             mbi.Protect & PAGE_READWRITE ||
             mbi.Protect & PAGE_EXECUTE_READ ||
             mbi.Protect & PAGE_EXECUTE_READWRITE))
        {
            memoryRegions[region.baseAddress] = region;
        }

        address += mbi.RegionSize;
    }

    return memoryRegions;
}

/**
 * @brief This method will check if a given address is valid
 * @param address -> This is the address to be checked
 * @param size -> This is the size of the given address
 * @return true , if the address is valid
 * @return false, if the address is not valid
 */
bool MemUtility::isAddressValid(DWORD address, size_t size)
{
    if (this->processHandle == nullptr)
    {
        throw std::runtime_error("Process is not open");
    }

    MEMORY_BASIC_INFORMATION memInfo;
    if (VirtualQueryEx(this->processHandle, reinterpret_cast<LPCVOID>(address), &memInfo, sizeof(memInfo)) == 0)
    {
        std::cerr << "VirtualQueryEx failed for address: " << std::hex << address << std::endl;
        return false;
    }

    std::cout << "BaseAddress: " << std::hex << memInfo.BaseAddress
              << ", RegionSize: " << std::dec << memInfo.RegionSize
              << ", State: " << memInfo.State
              << ", Protect: " << memInfo.Protect
              << ", Type: " << memInfo.Type << std::endl;

    if (memInfo.State != MEM_COMMIT)
    {
        std::cerr << "Address is not in committed state." << std::endl;
        return false;
    }

    bool isWritable = (memInfo.Protect & PAGE_READWRITE) ||
                      (memInfo.Protect & PAGE_EXECUTE_READWRITE) ||
                      (memInfo.Protect & PAGE_WRITECOPY) ||
                      (memInfo.Protect & PAGE_EXECUTE_WRITECOPY);

    if (!isWritable)
    {
        std::cerr << "Address does not have writable protection." << std::endl;
        return false;
    }

    uintptr_t regionStart = reinterpret_cast<uintptr_t>(memInfo.BaseAddress);
    uintptr_t regionEnd = regionStart + memInfo.RegionSize;
    if (address < regionStart || (address + size) > regionEnd)
    {
        std::cerr << "Address range is outside the memory region." << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief This method will automatically find a memeory region and write your given value to it
 * @param value This is the value which will be written into the mem
 */
void MemUtility::autoFindMemAndWrite(int value)
{
    if (this->processHandle == nullptr)
    {
        throw std::runtime_error("Process is not open");
    }

    std::map<uintptr_t, MemRegion> memoryRegions = getMemoryRegions();
    for (const auto& region : memoryRegions)
    {
        if (region.second.protect & PAGE_READWRITE)
        {
            std::cout << "Found writable memory region: " << std::hex << region.second.baseAddress << std::endl;

            if (region.second.regionSize < sizeof(int))
            {
                std::cerr << "Memory region is too small to write an integer" << std::endl;
                continue;
            }

            MEMORY_BASIC_INFORMATION mbi;
            if (!VirtualQueryEx(this->processHandle, reinterpret_cast<LPCVOID>(region.second.baseAddress), &mbi, sizeof(mbi)))
            {
                std::cerr << "VirtualQueryEx failed for address: " << std::hex << region.second.baseAddress << std::endl;
            }
            else if (WriteProcessMemory(this->processHandle, reinterpret_cast<LPVOID>(region.second.baseAddress), &value, sizeof(int), nullptr))
            {
                std::cout << "Wrote " << value << " to address: " << std::hex << region.second.baseAddress << std::endl;
                return;
            }
            else
            {
                std::cerr << "WriteProcessMemory failed for address: " << std::hex << region.second.baseAddress << std::endl;
            }
        }
    }

    std::cerr << "No writable memory regions found" << std::endl;
}