#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <stdlib.h>
#include <vector>
#include <Windows.h>

float const to_angle =  3.14159265/180;
bool is_cifr(char a);
int to_int(char c);
float read_op(std::istringstream& iss, float x);
float calculate(std::string& expr, float x);

float non_parallel(float a, float b, std::string& expr);


float parallel_winapi(float a, float b, std::string& expr, int num_threads);
struct ThreadData;
DWORD WINAPI ThreadFunction(LPVOID lpParam);



int main()
{
	SetConsoleOutputCP(1251);
	float a, b;
	//std::string str;
	//printf("Введите функцию \n(Поддерживаются lg, ln, cos, sin. \nOперации +,-,*,/.\nПрограмма не защищена от ошибок ввода, приоритет операций только через скобки)\nнапример: sin(9*x)+(2*x)-lg(x): ");
	//std::getline(std::cin, str);
	//printf("Введите пределы интегрирования: \na = "); std::cin >> a; printf("b = "); std::cin >> b;

	std::string str = "sin(x*9)+(2*x)-lg(x)";
	std::cout << "Non-parallel result: " << non_parallel(2, 5, str) << std::endl;

	std::cout << "Parallel result (WinAPI): " << parallel_winapi(2, 5, str, 5) << std::endl;
}



struct ThreadData {
	float start, end, step, * partial_result;
	std::string* expr;
};


DWORD WINAPI ThreadFunction(LPVOID lpParam) {
	ThreadData* data = (ThreadData*)lpParam;
	float res = 0.0f;
	float x = data->start + data->step / 2.0f;

	while (x < data->end) {
		res += calculate(*(data->expr), x);
		x += data->step;
	}

	*(data->partial_result) = res * data->step;
	return 0;
}

float parallel_winapi(float a, float b, std::string& expr, int num_threads) {
	if (a > b) std::swap(a, b);

	float total_result = 0.0f;
	float range = b - a;
	float thread_range = range / num_threads;
	float step = (b - a) / (5 * num_threads); // Сохраняем общее количество прямоугольников

	std::vector<HANDLE> threads(num_threads);
	std::vector<ThreadData> thread_data(num_threads);
	std::vector<float> partial_results(num_threads, 0.0f);

	// Создаем потоки
	for (int i = 0; i < num_threads; ++i) {
		thread_data[i].start = a + i * thread_range;
		thread_data[i].end = a + (i + 1) * thread_range;
		thread_data[i].expr = &expr;
		thread_data[i].step = step;
		thread_data[i].partial_result = &partial_results[i];

		threads[i] = CreateThread(
			NULL,                   // Атрибуты безопасности по умолчанию
			0,                      // Размер стека по умолчанию
			ThreadFunction,         // Функция потока
			&thread_data[i],       // Параметры потока
			0,                      // Флаги создания (по умолчанию)
			NULL                    // Идентификатор потока не возвращается
		);

		if (threads[i] == NULL) {
			std::cerr << "Error creating thread " << i << std::endl;
			return 0.0f;
		}
	}

	// Ожидаем завершения всех потоков
	WaitForMultipleObjects(num_threads, threads.data(), TRUE, INFINITE);

	// Собираем результаты
	for (int i = 0; i < num_threads; ++i) {
		total_result += partial_results[i];
		CloseHandle(threads[i]);
	}

	return total_result;
}

bool is_cifr(char a)
{
	return a >= '0' && a <= '9';
}

int to_int(char c)
{
	return c - '0';
}

float read_op(std::istringstream& iss, float x)
{
	std::string simple_expr;
	if (iss.peek() == 'l')//lg ln
	{
		iss.get();
		char second_letter = iss.get();
		iss.get();
		while (iss.peek() != ')')
			simple_expr += iss.get();
		iss.get();
		if (second_letter == 'n') return log(calculate(simple_expr, x));
		return log10(calculate(simple_expr, x));
	}
	else if (iss.peek() == 's')//sin
	{
		iss.get(); iss.get(); iss.get(); iss.get();
		while (iss.peek() != ')')
			simple_expr += iss.get();
		iss.get();
		return sin(calculate(simple_expr, x)*to_angle);
	}
	else if (iss.peek() == 'c')	//cos
	{
		iss.get(); iss.get(); iss.get(); iss.get();
		while (iss.peek() != ')')
			simple_expr += iss.get();
		iss.get();
		return cos(calculate(simple_expr, x) * to_angle);
	}
	else if (iss.peek() == '(')// скобки
	{
		iss.get(); 
		int balance = 1;
		while (balance != 0) {
			char c = iss.get();
			if (c == '(') balance++;
			else if (c == ')') balance--;
			if (balance != 0) simple_expr += c;
		}
		return calculate(simple_expr, x);
	}
	else if (iss.peek() == 'x') //x
	{
		iss.get();
		return x;
	}

	char c; int ch{ 0 }, mult = 1; std::string s_ch; //число
	do
	{
		if (is_cifr(iss.peek()))
		{
			c = iss.get();
			s_ch = c + s_ch;
		}
	} while (iss.peek() != EOF and is_cifr(iss.peek()));
	std::istringstream ss_ch(s_ch);
	while ((ss_ch.peek() != EOF))
	{
		ss_ch >> c;
		ch += to_int(c) * mult;
		mult *= 10;
	}
	return ch;
		

}

float calculate(std::string& expr, float x) {
	std::istringstream iss(expr);
	float result = 0;
	float current_value = read_op(iss, x);
	result = current_value;

	while (iss.peek() != EOF) {
		char op = iss.get();
		float next_value = read_op(iss, x);

		switch (op) {
		case '+': result += next_value; break;
		case '-': result -= next_value; break;
		case '*': result *= next_value; break;
		case '/': result /= next_value; break;
		default: break;
		}
	}

	return result;
}

float non_parallel(float a, float b, std::string& expr)
{
	if (a > b) std::swap(a, b);
	float res{}, step = (b - a)/5, cur_pos = a, heigh, x;

	while (abs(cur_pos - b) > 0.001)
	{
		x = cur_pos + step / 2;
		heigh = calculate(expr, x);
		res += heigh;
		cur_pos += step;
	}
	res *= step;

	return res;
}

