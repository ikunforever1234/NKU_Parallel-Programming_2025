#include<iostream>
#include<Windows.h>
using namespace std;
const int N = 1000;
double a[N][N], b[N], sum[N];
void initial() {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			a[i][j] = i + j;
		}
		b[i] = i;
	}
}

//m为程序执行次数
void gettime_normal(int m) {
	initial();
	long long head, tail, freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);
	//设置执行次数
	for (int f = 0; f < m; f++) {
		
		for (int i = 0; i < N; i++) {
			sum[i] = 0;
			for (int j = 0; j < N; j++) {
				sum[i] += a[j][i] * b[j];
			}
		}

	}
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);
	double time_normal = (tail - head) * 1000.0 / freq;
	cout<< "执行"<<m<<"次normal时间: " << time_normal << " ms" << endl;
}

void gettime_cache(int m) {
	initial();
	long long head, tail, freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);
	//设置执行次数
	for (int f = 0; f < m; f++) {

		for (int i = 0; i < N; i++) {
			sum[i] = 0;
		}
		for (int i = 0; i < N; i++) {
			for (int j = 0; j < N; j++) {
				sum[j] += a[i][j] * b[i];
			}
		}

	}
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);
	double time_cache = (tail - head) * 1000.0 / freq;
	cout << "执行" << m << "次cache时间: " << time_cache << " ms" << endl;
}
int main() {
	int m;
	cout << "输入执行次数：" << endl;
	cin >> m;

	gettime_normal(m);
	gettime_cache(m);
	
	return 0;
}