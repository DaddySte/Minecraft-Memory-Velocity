#include "Velocity.h"

//Set to Multibyte
HANDLE checkMinecraftHandle() {
    HANDLE pHandle = NULL;

    while (!pHandle) {

        if (HWND hwnd = FindWindow("LWJGL", NULL)) {

            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            pHandle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return pHandle;
}

int CustomRandomNumber(int min, int max) {

    std::random_device random;
    std::uniform_int_distribution<int> dist(min, max);

    return dist(random);
}

void DoubleScanner(double target_value, std::vector<uint64_t>& memory_addresses) {
    MEMORY_BASIC_INFORMATION memInfo = {};
    SYSTEM_INFO sysInfo = {};
    GetSystemInfo(&sysInfo);

    LPVOID currentAddress = sysInfo.lpMinimumApplicationAddress;
    LPVOID maxAddress = sysInfo.lpMaximumApplicationAddress;

    while (currentAddress < maxAddress) {

        if (VirtualQueryEx(MINECRAFT_HANDLE, currentAddress, &memInfo, sizeof(memInfo))) {
            if ((memInfo.State == MEM_COMMIT) && ((memInfo.Protect & PAGE_GUARD) == 0) && ((memInfo.Protect & PAGE_EXECUTE) ||
                (memInfo.Protect & PAGE_EXECUTE_READ) || (memInfo.Protect & PAGE_EXECUTE_READWRITE) ||
                (memInfo.Protect & PAGE_EXECUTE_WRITECOPY))) {
                SIZE_T bytesRead;
                std::vector<double> buffer(memInfo.RegionSize / sizeof(double));

                if (ReadProcessMemory(MINECRAFT_HANDLE, currentAddress, buffer.data(), memInfo.RegionSize, &bytesRead)) {
                    uint64_t addressOffset = 0;
                    for (size_t i = 0; i < bytesRead / sizeof(double); i++) {
                        if (buffer[i] == target_value) {
                            LPBYTE address = (LPBYTE)currentAddress + addressOffset;
                            if (std::find(memory_addresses.begin(), memory_addresses.end(), (uint64_t)address) == memory_addresses.end()) {
                                memory_addresses.push_back((uint64_t)address);
                            }
                        }
                        addressOffset += sizeof(double);
                    }
                }
            }
            currentAddress = (LPBYTE)memInfo.BaseAddress + memInfo.RegionSize;
        }
        else {
            currentAddress = (LPBYTE)currentAddress + sysInfo.dwPageSize;
        }
    }
}

void BytesScanner(uint64_t start_address, std::vector<BYTE> search_bytes, int num_addresses, std::vector<uint64_t>& bytes_location) {

	for (int i = 0; i < num_addresses; i++) {

		uint64_t address = start_address + i;
		BYTE buffer;

		for (int j = 0; j < search_bytes.size(); j++) {
			if (search_bytes[j] != 0x00) {
				ReadProcessMemory(MINECRAFT_HANDLE, (LPCVOID)(address + j), &buffer, sizeof(buffer), NULL);
				if (buffer != search_bytes[j]) break;
				if (j == search_bytes.size() - 1) {
					bytes_location.push_back(address);
				}
			}
		}
	}
}

struct {
	double velocity_value = 8000;
	std::vector<uint64_t> first_start_addresses;
	std::vector<uint64_t> memory_velocity_addresses;
	std::vector<BYTE> velocity_bytes_one = { 0xC5, 0x00, 0x5E, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xC5, 0x00, 0x5E, 0x00, 0x00,
		0xFE, 0xFF, 0xFF, 0xC5, 0x00, 0x5E, 0x00, 0x00, 0xFE, 0xFF, 0xFF };
	std::vector<uint64_t> bytes_location;
	std::vector<uint64_t> xyz_velocity;
}memoryVelocity;

//Read or write if they point to the same address
uint64_t AddressValueCalculator(uint64_t base_address, int offset, bool rewrite) {
	BYTE buffer[2];

	ReadProcessMemory(MINECRAFT_HANDLE, (LPCVOID)(base_address + 4 + offset), &buffer[0], sizeof(buffer[0]), NULL);
	ReadProcessMemory(MINECRAFT_HANDLE, (LPCVOID)(base_address + 5 + offset), &buffer[1], sizeof(buffer[1]), NULL);
	unsigned short combined_value = (buffer[1] << 8) | buffer[0];

	if (!rewrite) {
		uint64_t new_calculated_address = base_address + offset - 0xFFF8 + combined_value;
		return new_calculated_address;
	}
	else {
		combined_value += offset;

		BYTE byte1 = (combined_value >> 8) & 0xFF;
		BYTE byte2 = combined_value & 0xFF;

		WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)(base_address + 4 + offset), &byte2, sizeof(byte2), NULL);
		WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)(base_address + 5 + offset), &byte1, sizeof(byte1), NULL);
		uint64_t new_calculated_address = base_address + offset - 0xFFF8 + combined_value;
		WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)new_calculated_address, &memoryVelocity.velocity_value, sizeof(double), NULL);
		return new_calculated_address;
	}
}

void VelocityScanner() {
	while (true) {

		//Find addresses
		DoubleScanner(memoryVelocity.velocity_value, memoryVelocity.memory_velocity_addresses);

		//Deletes values different from basic velo value or the one we changed them in
		std::sort(memoryVelocity.memory_velocity_addresses.begin(), memoryVelocity.memory_velocity_addresses.end());

		for (int i = 0; i < memoryVelocity.memory_velocity_addresses.size(); i++) {
			double buffer_reader = 0;
			ReadProcessMemory(MINECRAFT_HANDLE, (LPCVOID)memoryVelocity.memory_velocity_addresses[i], &buffer_reader, sizeof(buffer_reader), NULL);
			if ((buffer_reader != memoryVelocity.velocity_value)) {
				memoryVelocity.memory_velocity_addresses.erase(memoryVelocity.memory_velocity_addresses.begin() + i);
				i--;
			}
		}

		//Delete repetitions
		memoryVelocity.memory_velocity_addresses.erase(std::unique(memoryVelocity.memory_velocity_addresses.begin(), memoryVelocity.memory_velocity_addresses.end()), memoryVelocity.memory_velocity_addresses.end());

		if (memoryVelocity.memory_velocity_addresses.size() == 3) {
			if (memoryVelocity.first_start_addresses.size() != 3) {
				memoryVelocity.first_start_addresses = memoryVelocity.memory_velocity_addresses;
			}
		}
		else {
			//Find Instructions
			for (int i = 0; i < memoryVelocity.memory_velocity_addresses.size(); i++)
				BytesScanner(memoryVelocity.memory_velocity_addresses[i], memoryVelocity.velocity_bytes_one, 500, memoryVelocity.bytes_location);

			//Delete repetitions
			std::sort(memoryVelocity.bytes_location.begin(), memoryVelocity.bytes_location.end());
			memoryVelocity.bytes_location.erase(std::unique(memoryVelocity.bytes_location.begin(), memoryVelocity.bytes_location.end()), memoryVelocity.bytes_location.end());

			for (int i = 0; i < memoryVelocity.bytes_location.size(); i++) {
				std::vector<uint64_t> bytes_location;

				for (int j = 0; j < 24; j += 8) {
					bytes_location.push_back(AddressValueCalculator(memoryVelocity.bytes_location[i], j, false));
				}

				if (bytes_location[0] == bytes_location[1] || bytes_location[0] == bytes_location[2] || bytes_location[1] == bytes_location[2]) {
					memoryVelocity.xyz_velocity.push_back(bytes_location[0]);
					memoryVelocity.xyz_velocity.push_back(AddressValueCalculator(memoryVelocity.bytes_location[i], 8, true));
					memoryVelocity.xyz_velocity.push_back(AddressValueCalculator(memoryVelocity.bytes_location[i], 16, true));
				}
				else {
					std::sort(memoryVelocity.xyz_velocity.begin(), memoryVelocity.xyz_velocity.end());
					for (int k = 0; k < bytes_location.size(); k++) {
						memoryVelocity.xyz_velocity.push_back(bytes_location[k]);
					}
					memoryVelocity.xyz_velocity.erase(std::unique(memoryVelocity.xyz_velocity.begin(), memoryVelocity.xyz_velocity.end()), memoryVelocity.xyz_velocity.end());
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	}
}

void changeVelocity(int min_h_val, int max_h_val, int min_v_val, int max_v_val, int chance, bool negative, bool increase, bool while_sprinting) {
	double new_h_val = 8000;
	double new_v_val = 8000;

	if ((while_sprinting && GetAsyncKeyState('W') & 0x8000) || !while_sprinting) {
		if (chance > CustomRandomNumber(0, 99)) {
			new_v_val = 8000 + ((100 - CustomRandomNumber(min_v_val, max_v_val)) * 400);

			if (negative) {
				new_h_val = -(CustomRandomNumber(min_h_val, max_h_val) * 70) - 1000;
			}
			else if (increase) {
				new_h_val = 8000 - ((100 - CustomRandomNumber(min_h_val, max_h_val)) * 70);
				new_v_val = 8000 - ((100 - CustomRandomNumber(min_v_val, max_v_val)) * 70);
			}
			else {
				new_h_val = 8000 + ((100 - CustomRandomNumber(min_h_val, max_h_val)) * 400);
			}
		}
		else {
			new_h_val = 8000;
			new_v_val = 8000;
		}
	}

	if (memoryVelocity.xyz_velocity.empty() && !memoryVelocity.first_start_addresses.empty()) {
		WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)memoryVelocity.first_start_addresses[0], &new_h_val, sizeof(new_h_val), NULL);
		WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)memoryVelocity.first_start_addresses[1], &new_v_val, sizeof(new_v_val), NULL);
		WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)memoryVelocity.first_start_addresses[2], &new_h_val, sizeof(new_h_val), NULL);
	}

	for (int i = 0; i < memoryVelocity.xyz_velocity.size(); i += 3) {
		WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)memoryVelocity.xyz_velocity[i], &new_h_val, sizeof(new_h_val), NULL);
		WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)memoryVelocity.xyz_velocity[i + 1], &new_h_val, sizeof(new_h_val), NULL);
		WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)memoryVelocity.xyz_velocity[i + 2], &new_v_val, sizeof(new_v_val), NULL);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(MINECRAFT_TICK));
}

/* How to use
* Run Velocity scanner in a different thread. Jvm keeps changing addresses so the loop must keep running as long as the program is opened (Infinite loop)
* When you first start minecraft jvm loads velocity x,y,z in different addresses. When you scan for the 8000 double value you'll find only those values, so 3 addresses.
*  that are saved in first_start_addresses. first address is x, second is y, third is z
* After a while x,y,z doulbe values will be united in a single address (Use cheat engine to check)
* When velocity scanner detects that AddressValueCalculator finds the bytecode and splits x,y and z in 3 different addresses that are saved in memoryVelocity.xyz_velocity
* first value is x, second is z, third is y
* 
* Negative can be used to invert velocity. Increase to add more velocity
*/