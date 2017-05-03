#include <vector>;
#include <fstream>;
#include <istream>;
#include <iterator>;
#include <cstdint>;
#include <iostream>;
#include <cassert>;
typedef unsigned char BYTE;
// #define MAX_OPERATION 120; TODO why did this not work?

#pragma pack(push, 1)
struct EncryptionStepDescriptor
{
	unsigned char operationCode;
	unsigned char operationParameter;
	unsigned int lengthToOperateOn;
};
#pragma pack(pop)

std::vector<BYTE> readFile(const char* filename)
{
	// open the file:
	std::ifstream file(filename, std::ios::binary);

	// Stop eating new lines in binary mode!!!
	file.unsetf(std::ios::skipws);

	// get its size:
	std::streampos fileSize;

	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	// reserve capacity
	std::vector<BYTE> vec;
	vec.reserve(fileSize);

	// read the data:
	vec.insert(vec.begin(),
		std::istream_iterator<BYTE>(file),
		std::istream_iterator<BYTE>());

	return vec;
}

std::vector<EncryptionStepDescriptor> convertDescriptors(const std::vector<BYTE> &bVector) {
	EncryptionStepDescriptor sample;
	const int num = bVector.size() / sizeof sample;
	std::vector<EncryptionStepDescriptor> ret;
	int counter = 0;
	for (int i = num; i > 0; i--) {
		memcpy(&sample, &bVector[counter], sizeof sample);
		ret.push_back(sample);
		counter += sizeof sample;
	}
	return ret;
}

std::vector<BYTE> decrypt(std::vector<EncryptionStepDescriptor> &descriptors, std::vector<BYTE> msg) {
	// set marker at 0
	int marker = 0;
	bool reverse = false;
	// for each descriptor
	for (auto d : descriptors) {
		// for counter of times to execute
		for (int i = d.lengthToOperateOn; i > 0; i--) {
			// perform operation on cur byte
			switch (d.operationCode) {
			case 0:
				msg[marker] ^= d.operationParameter;
				break;
			case 1:
				msg[marker] += d.operationParameter;
				break;
			case 2:
				msg[marker] -= d.operationParameter;
				break;
			}
			// step marker:
			// if last byte and not reverse:
			if (marker == (msg.size() - 1) && !reverse) {
				reverse = true;
			}
			// if first byte and reverse:
			else if (marker == 0 && reverse) {
				reverse = false;
			}
			else if (reverse) {
				marker--;
			}
			else marker++;
		}
	}

	std::cout << "decrypted with key: TODO write key\n";
	return msg;
}
/*
bool isValidChar(BYTE c) {
	if ((65 <= (int)c && (int)c <= 90) ||
		(97 <= (int)c && (int)c <= 122) ||
		((int)c == 46) ||
		((int)c == 63) ||
		((int)c == 39))
		return true;

	return false;
}
*/

bool isValidChar(BYTE c) {
	if (('a' <= c && c <= 'z') ||
		('A' <= c && c <= 'Z') ||
		(c == '.') ||
		(c == '?') ||
		(c == ','))
		return true;

	return false;
}

void solveCurrentChar(const short& msgSize, int& marker, bool& reverse, const EncryptionStepDescriptor& d, BYTE& currentChar, const int& charIndex) {
	//int pos = marker;
	
	int applyMultiplier = 0; // how many times should the operation be performed on this cell 

	// pass on first round?
	if ((!reverse && marker + d.lengthToOperateOn > charIndex && marker <= charIndex) || (reverse && (int) (marker - d.lengthToOperateOn) < charIndex && marker >= charIndex)) {
		// we will step on the cur char index on the first pass by
		applyMultiplier = 1;
	}
	if ((!reverse && d.lengthToOperateOn + marker < msgSize) || (reverse && d.lengthToOperateOn < marker)) {
		// no direction change
		marker = reverse ? marker - d.lengthToOperateOn : marker + d.lengthToOperateOn;
		if (applyMultiplier == 0) return; // nothing else to do this round
	}
	else {
		// assuming at least one direction change
		int L = reverse ? (d.lengthToOperateOn - marker) : (int)(d.lengthToOperateOn - (msgSize - marker)); // lengthToOperate not including steps of the first pass
		int dc =  L/msgSize; // how many direction changes do we have? (not including first one)
		if (dc + 1 % 2 == 1) reverse = !reverse; // odd number of direction changes means we need to change the final direction. adding one for the first direction change
		applyMultiplier += dc; // each time we do a direction change that includes a full pass on the vector we will also need to do the calculation again
		auto leftOvers = L % (msgSize);
		assert(0 <= leftOvers && leftOvers < msgSize);
		// determine if leftOvers result in another step on charIndex
		if (reverse) {
			//set marker
			marker = msgSize - 1 - leftOvers;
			if (marker < charIndex) {
				// passed the index on the last run too
				applyMultiplier++;
			}
		}
		else {
			//set marker
			marker = leftOvers;
			if (marker > charIndex) {
				// passed the index on the last run too
				applyMultiplier++;
			}
		}
	} // end direction change logic

	if (applyMultiplier == 0) return; // try to save on the switch? TODO check if this is worth while

	switch (d.operationCode) {
	case 0:
		for (int i = applyMultiplier; i > 0; i--) {
			currentChar ^= d.operationParameter;
		}
		break;
	case 1:
		currentChar += d.operationParameter * applyMultiplier;
		break;
	case 2:
		currentChar -= d.operationParameter * applyMultiplier;
		break;
	}
}

bool decryptAndTest(const std::vector<EncryptionStepDescriptor> &descriptors,const std::vector<BYTE>& msg, std::vector<BYTE>& res) {
	// set marker at 0
	int marker = 0;
	bool reverse = false;
	short msgSize = msg.size();
	BYTE currentChar;
	// for each letter
	for (int currentIndex = 0; currentIndex < msgSize; currentIndex++) {
		currentChar = msg[currentIndex];
		// for each descriptor
		for (auto d : descriptors) {
			solveCurrentChar(msgSize, marker, reverse, d, currentChar, currentIndex);
		} // end descriptors cycle
		if (!isValidChar(currentChar))
			return false;

		res[marker] = currentChar;
	}
	return true;
}

bool printVector(const std::vector<BYTE> &msg) {
	for (std::vector<BYTE>::const_iterator i = msg.begin(); i != msg.end(); ++i) {
		if (isValidChar(*i))
			std::cout << *i;
		else {
			std::cout << "failed printing on: " << *i << ":" << (int)*i <<'\n';
			return false;
		}
	}
	std::cout << '\n';
	return true;
}

int main() {
	EncryptionStepDescriptor sample;
	
	// get binary descriptors
	const char KeyFile[] = "E:/MakeMeCompile/Key.bin";
	std::vector<BYTE> vec = readFile(KeyFile);
	const char MsgFile[] = "E:/MakeMeCompile/EncryptedMessage2.bin";
	// get binary message
	std::vector<BYTE> msg = readFile(MsgFile);
	
	// get descriptors
	// from file:
	//std::vector<EncryptionStepDescriptor> descriptors = convertDescriptors(vec);
	
	// BRUTE FORCE
	int counter = 0;
	short MAX_OPERATION = 150;
	short MAX_LENGTH = 50;
	std::vector<EncryptionStepDescriptor> descriptors;
	std::vector<BYTE> res;
	res.reserve(msg.size());
	descriptors.reserve(3);
	for (short i = 0; i < 3; i++)
		descriptors.push_back(sample);
	EncryptionStepDescriptor *A, *B, *C;
	// link pointers
	A = & descriptors[0];
	B = & descriptors[1];
	C = & descriptors[2];
	// A rep
	for (uint32_t ar = 1; ar < MAX_LENGTH; ar++) {
		A->lengthToOperateOn = ar;
		// B rep
		for (uint32_t br = 1; br < MAX_LENGTH; br++) {
			B->lengthToOperateOn = br;
			// C rep
			for (uint32_t cr = 1; cr < MAX_LENGTH; cr++) {
				C->lengthToOperateOn = cr;
				if (ar + br + cr < msg.size()) continue;
				// A value
				for (unsigned char av = 0; av < MAX_OPERATION; av++) {
					A->operationParameter = av;
					// B value
					for (unsigned char bv = 0; bv < MAX_OPERATION; bv++) {
						B->operationParameter = bv;
						// C value
						for (unsigned char cv = 0; cv < MAX_OPERATION; cv++) {
							C->operationParameter = cv;
							// A operator
							for (short ao = 0; ao < 3; ao++) {
								A->operationCode = ao;
								// B operator
								for (short bo = 0; bo < 3; bo++) {
									if (bo == ao) continue;
									B->operationCode = bo;
									// C operator
									for (short co = 0; co < 3; co++) {
										if (co == bo) continue;
										C->operationCode = co;

										// LETS DO IT
										counter++;
										if (counter % 1000 == 0) {
											std::cout << "Attempting with keys: A{"
												<< +descriptors[0].operationCode << "," << +descriptors[0].operationParameter << "," << +descriptors[0].lengthToOperateOn
												<< "}, B{"
												<< +descriptors[1].operationCode << "," << +descriptors[1].operationParameter << "," << +descriptors[1].lengthToOperateOn
												<< "}, C{"
												<< +descriptors[2].operationCode << "," << +descriptors[2].operationParameter << "," << +descriptors[2].lengthToOperateOn << "}\n";
										}
										if (decryptAndTest(descriptors, msg, res) && printVector(res)) {
											std::cout << "\n\n======== NICE =========";
											return 0;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	std::cout << "FAIL";	
}