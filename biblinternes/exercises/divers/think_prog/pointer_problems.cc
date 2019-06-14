#include <iostream>

typedef char* arrayString;

char characterAt(arrayString &s, int position)
{
	return s[position];
}

int length(const arrayString s)
{
	int count = 0;

	while (s[count] != 0) {
		count++;
	}

	return count;
}

void append(arrayString &s, char c)
{
	int oldLength = length(s);

	arrayString newS = new char[oldLength + 2];

	for (int i = 0; i < oldLength; ++i) {
		newS[i] = s[i];
	}

	newS[oldLength] = c;
	newS[oldLength + 1] = 0;

	delete[] s;

	s = newS;
}

void concatenate(arrayString &s1, const arrayString s2)
{
	int s1_oldLength = length(s1);
	int s2_length = length(s2);
	int s1_newLength = s1_oldLength + s2_length;
	arrayString newS = new char[s1_newLength + 1];

	for (int i = 0; i < s1_oldLength; ++i) {
		newS[i] = s1[i];
	}

	for (int i = 0; i < s2_length; ++i) {
		newS[s1_oldLength + i] = s2[i];
	}

	newS[s1_newLength] = 0;

	delete[] s1;

	s1 = newS;
}

arrayString substring(const arrayString s, int pos, int length)
{
	arrayString sub_str = new char[length + 1];

	for (int i = 0, ch = pos - 1; i < pos + length; ++i, ++ch) {
		sub_str[i] = s[ch];
	}

	sub_str[length] = 0;

	return sub_str;
}

void replaceString(arrayString &source,
				   const arrayString target,
				   const arrayString replaceText)
{
	(void)source, (void)target, (void)replaceText;

	int pos = -1, target_length = length(target);

	for (int i = 0; i < length(source); ++i) {
		for (int j = i; j < target_length; ++j) {
			if (target[j] == source[j]) {
				pos = j;
			}
		}
	}
}

int main()
{
	arrayString str, substr;
	str = new char[7];

	str[0] = 'a', str[1] = 'b', str[2] = 'c', str[3] = 'd', str[4] = 'e', str[5] = 'f', str[6] = 'g';

	substr = substring(str, 3, 4);

//	for (int i = 0; i < length(substr); ++i) {
		std::cout << substr;
//	}
	std::cout << std::endl;

	concatenate(str, substr);

	std::cout << str << std::endl;

	return 0;
}
