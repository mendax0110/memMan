#include "../include/MemUtility.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <iterator>

void printMemoryRegions(const std::map<uintptr_t, MemRegion>& memoryRegions)
{
    std::cout << std::setw(18) << "Base Address"
                << std::setw(14) << "Region Size"
                << std::setw(10) << "State"
                << std::setw(12) << "Protect"
                << std::setw(8) << "Type" << std::endl;

    for (const auto& region : memoryRegions)
    {
        std::cout << std::hex << std::setw(18) << region.second.baseAddress
                    << std::dec << std::setw(14) << region.second.regionSize
                    << std::setw(10) << region.second.state
                    << std::setw(12) << region.second.protect
                    << std::setw(8) << region.second.type << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <process name> <address> [-r | -w | -l | -a] [value]" << std::endl;
        return 1;
    }

    try
    {
        MemUtility memUtil(argv[1]);
        memUtil.openProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION);

        if (strcmp(argv[2], "-l") == 0)
        {
            auto memoryRegions = memUtil.getMemoryRegions();
            printMemoryRegions(memoryRegions);
        }
        else if (strcmp(argv[2], "-a") == 0)
        {
            int value = std::stoi(argv[3]);
            memUtil.autoFindMemAndWrite(value);
        }
        else
        {
            uintptr_t address = std::stoull(argv[2], nullptr, 16);

            if (argc == 3 || (argc == 4 && strcmp(argv[3], "-r") == 0))
            {
                if (!memUtil.isAddressValid(address, sizeof(int)))
                {
                    std::cerr << "Invalid address: " << std::hex << address << std::endl;
                }
                memUtil.readMemory(address, sizeof(int));
                std::cout << "Value at address " << std::hex << address << ": " << sizeof(int) << std::endl;
            }
            else if (argc == 5 && strcmp(argv[3], "-w") == 0)
            {
                int value = std::stoi(argv[4]);
                if (!memUtil.isAddressValid(address, sizeof(int)))
                {
                    std::cerr << "Invalid address: " << std::hex << address << std::endl;
                }
                memUtil.writeMemory(&value, address, sizeof(int));
                std::cout << "Wrote " << value << " to address " << std::hex << address << std::endl;
            }
            else
            {
                std::cerr << "Usage: " << argv[0] << " <process name> <address> [-r | -w] [value]" << std::endl;
                return 1;
            }
        }
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "Invalid argument: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::out_of_range& e)
    {
        std::cerr << "Argument out of range: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}