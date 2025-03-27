#include<iostream>
#include<Windows.h>
using namespace std;

const long long int n = (1<<21);

double a[n];
long long int result = 0;

void initial() {
	for (long long int i = 0; i < n; i++) {
		a[i] = i;
	}
}


void gettime_normal(int m) {
	long long head, tail, freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);

	for (int f = 0; f < m; f++) {
		result = 0;
		for (int i = 0; i < n; i++) {
			result += a[i];
		}
	}

	QueryPerformanceCounter((LARGE_INTEGER*)&tail);
	double time_normal = (tail - head) * 1000.0 / freq;
	cout << "normal时间: " << time_normal << " ms" << endl;
	cout << result << endl;
}

void gettime_pro(int m) {
	long long head, tail, freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);

	for (int f = 0; f < m; f++) {
		result = 0;
		long long int result1 = 0, result2 = 0;
		for (int i = 0; i < n; i += 2) {
			result1 += a[i];
			result2 += a[i + 1];
		}
		result = result1 + result2;
	}

	QueryPerformanceCounter((LARGE_INTEGER*)&tail);
	double time_pro = (tail - head) * 1000.0 / freq;
	cout << "pro时间: " << time_pro << " ms" << endl;
	cout << result << endl;
}

int main() {
	initial();
	int m;
	cout << "输入执行次数：" << endl;
	cin >> m;
	gettime_normal(m);
	gettime_pro(m);

	return 0;
}
