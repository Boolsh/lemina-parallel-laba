#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <thread>
#include <queue>
#include <mutex>
#include <future>
#include <stdlib.h>
#include <vector>
#include <Windows.h>

const int rectangle_cnt = 10;


float const to_angle =  3.14159265/180;
bool is_cifr(char a);
int to_int(char c);
float read_op(std::istringstream& iss, float x);
float calculate(std::string& expr, float x);
float not_parallel(float a, float b, std::string& expr);


float parallel_winapi(float a, float b, std::string& expr, int num_threads);
struct ThreadDataW;
DWORD WINAPI ThreadFunctionW(LPVOID lpParam);

struct ThreadDataT;
void ThreadFunctionT(ThreadDataT& data);
float parallel_std_thread(float a, float b, std::string& expr, int num_threads);

float parallel_future(float a, float b, std::string& expr, int num_threads);
float calculate_part(float start, float end, std::string& expr, float step);

struct ThreadDataIL;
void ThreadFunctionInterlocked(ThreadDataIL& data);
float parallel_interlocked(float a, float b, std::string& expr, int num_threads);

float parallel_threadpool(float a, float b, std::string& expr, int num_threads);

int main()
{
	SetConsoleOutputCP(1251);
	float a{ 1 }, b{9};
	//std::string str;
	//printf("Введите функцию \n(Поддерживаются lg, ln, cos, sin. \nOперации +,-,*,/.\nПрограмма не защищена от ошибок ввода, приоритет операций только через скобки)\nнапример: sin(9*x)+(2*x)-lg(x): ");
	//std::getline(std::cin, str);
	//printf("Введите пределы интегрирования: \na = "); std::cin >> a; printf("b = "); std::cin >> b;

	std::string str = "sin(x*9)+(2*x)-lg(x)";
	std::cout << "Non-parallel result: " << not_parallel(a, b, str) << std::endl;
	std::cout << "Parallel result (WinAPI): " << parallel_winapi(a, b, str, rectangle_cnt/5) << std::endl;
	std::cout << "Parallel result (std::threads): " << parallel_std_thread(a, b, str, rectangle_cnt / 5) << std::endl;
	std::cout << "Parallel result (Future): " << parallel_future(a, b, str, rectangle_cnt / 5) << std::endl;
	std::cout << "Parallel result (Inter): " << parallel_interlocked(a, b, str, rectangle_cnt / 5) << std::endl;
	std::cout << "Parallel result (Pool): " << parallel_threadpool(a, b, str, rectangle_cnt / 5) << std::endl;
}


//WinAPI
struct ThreadDataW {
	float start, end, step, *partial_result;
	std::string* expr;
};
DWORD WINAPI ThreadFunctionW(LPVOID lpParam) 
{
	ThreadDataW* data = (ThreadDataW*)lpParam;
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
	float step = (b - a) / (rectangle_cnt * num_threads); // Сохраняем общее количество прямоугольников

	std::vector<HANDLE> threads(num_threads);
	std::vector<ThreadDataW> thread_data(num_threads);
	std::vector<float> partial_results(num_threads, 0.0f);


	for (int i = 0; i < num_threads; ++i) {
		thread_data[i].start = a + i * thread_range;
		thread_data[i].end = a + (i + 1) * thread_range;
		thread_data[i].expr = &expr;
		thread_data[i].step = step;
		thread_data[i].partial_result = &partial_results[i];

		threads[i] = CreateThread(
			NULL,                  
			0,                     
			ThreadFunctionW,        //Функция, которую делает поток
			&thread_data[i],       //параметры для этой функции
			0,                      
			NULL                    
		);

		if (threads[i] == NULL) {
			std::cerr << "Error creating thread " << i << std::endl;
			return 0.0f;
		}
	}
	WaitForMultipleObjects(num_threads, threads.data(), true, INFINITE);

	for (int i = 0; i < num_threads; ++i) {
		total_result += partial_results[i];
		CloseHandle(threads[i]);
	}

	return total_result;
}
//Threads
struct ThreadDataT {
	float start;
	float end;
	std::string* expr;
	float step;
	float partial_result;  
};
void ThreadFunctionT (ThreadDataT& data) 
{
	float res = 0.0f;
	float x = data.start + data.step / 2.0f;

	while (x < data.end) {
		res += calculate(*(data.expr), x);
		x += data.step;
	}

	data.partial_result = res * data.step;
}
float parallel_std_thread(float a, float b, std::string& expr, int num_threads) {
	if (a > b) std::swap(a, b);

	float total_result = 0.0f;
	float range = b - a;
	float thread_range = range / num_threads;
	float step = (b - a) / (5 * num_threads);

	std::vector<std::thread> threads;
	std::vector<ThreadDataT> thread_data(num_threads);

	// Создаем и запускаем потоки
	for (int i = 0; i < num_threads; ++i) {
		thread_data[i] = {
			a + i * thread_range,       // start
			a + (i + 1) * thread_range, // end
			&expr,                      // expr
			step,                       // step
			0.0f                       // partial_result (инициализируем 0)
		};

		threads.emplace_back(ThreadFunctionT, std::ref(thread_data[i]));
	}

	// Ждем завершения всех потоков
	for (auto& t : threads) {
		t.join();
	}

	// Собираем результаты
	for (auto& data : thread_data) {
		total_result += data.partial_result;
	}

	return total_result;
}
//future
float calculate_part(float start, float end, std::string& expr, float step) {
	float res = 0.0f;
	float x = start + step / 2.0f;

	while (x < end) {
		res += calculate(expr, x);
		x += step;
	}

	return res * step;
}
float parallel_future(float a, float b, std::string& expr, int num_threads) {
	if (a > b) std::swap(a, b);

	float total_result = 0.0f;
	float range = b - a;
	float thread_range = range / num_threads;
	float step = range / (5 * num_threads);

	std::vector<std::future<float>> futures;

	for (int i = 0; i < num_threads; ++i) 
	{
		float start = a + i * thread_range;
		float end = a + (i + 1) * thread_range;

		futures.emplace_back(std::async(std::launch::async, calculate_part,start, end, std::ref(expr), step));
		//std::future<float> f = std::async(std::launch::async, calculate_part( start, end, std::ref(expr), step))
	}


	for (auto& future : futures)
		total_result += future.get();

	return total_result;
}
//Атомарные типы
struct ThreadDataIL {
	float start;
	float end;
	std::string* expr;
	float step;
	volatile LONG* total_result;  // Для Interlocked-операций
};
void ThreadFunctionInterlocked(ThreadDataIL& data) {
	float res = 0.0f;
	float x = data.start + data.step / 2.0f;

	while (x < data.end) {
		res += calculate(*(data.expr), x);
		x += data.step;
	}

	float partial_result = res * data.step;

	// Атомарное добавление к общему результату
	InterlockedExchangeAdd(data.total_result, static_cast<LONG>(partial_result * 1000));
}
float parallel_interlocked(float a, float b, std::string& expr, int num_threads) {
	if (a > b) std::swap(a, b);

	volatile LONG total_result = 0;  // Используем LONG для Interlocked-операций
	float range = b - a;
	float thread_range = range / num_threads;
	float step = range / (5 * num_threads);

	std::vector<std::thread> threads;
	std::vector<ThreadDataIL> thread_data(num_threads);

	// Создаем и запускаем потоки
	for (int i = 0; i < num_threads; ++i) {
		thread_data[i] = {
			a + i * thread_range,
			a + (i + 1) * thread_range,
			&expr,
			step,
			&total_result
		};

		threads.emplace_back(ThreadFunctionInterlocked, std::ref(thread_data[i]));
	}
	for (auto& t : threads) {
		t.join();
	}
	return static_cast<float>(total_result) / 1000.0f;
}
// Pool
struct ThreadDataP 
{
	float start;
	float end;
	std::string expr;
	float step;
	float* result;  // Указатель для сохранения результата
};

// Потокобезопасная очередь на мьютексе и условной переменной
class SafeQueue {
	std::queue<ThreadDataP> queue;
	std::mutex mtx;
	std::condition_variable cv;
	bool shutdown = false;

public:
	void push(ThreadDataP data) {
		std::lock_guard<std::mutex> lock(mtx);
		queue.push(data);
		cv.notify_one();
	}

	bool pop(ThreadDataP& data) {
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this]() { return !queue.empty() || shutdown; });

		if (shutdown && queue.empty()) {
			return false;
		}

		data = queue.front();
		queue.pop();
		return true;
	}

	void shutdownQueue() {
		std::lock_guard<std::mutex> lock(mtx);
		shutdown = true;
		cv.notify_all();
	}
};


// Пул потоков для вычисления интеграла
float parallel_threadpool(float a, float b, std::string& expr, int num_threads) {
	if (a > b) std::swap(a, b);

	float total_result = 0.0f;
	float range = b - a;
	float thread_range = range / num_threads;
	float step = range / (5 * num_threads);

	SafeQueue data_queue;
	std::vector<float> partial_results(num_threads);
	std::vector<std::thread> pool;

	// Рабочая функция для потоков
	auto worker = [&]() {
		ThreadDataP data;
		while (data_queue.pop(data)) {
			*data.result = calculate_part(data.start, data.end, data.expr, data.step);
		}
		};

	// Создаем пул потоков
	for (int i = 0; i < num_threads; ++i) {
		pool.emplace_back(worker);
	}

	// Добавляем задачи в очередь
	for (int i = 0; i < num_threads; ++i) {
		ThreadDataP data = {
			a + i * thread_range,
			a + (i + 1) * thread_range,
			expr,
			step,
			&partial_results[i]
		};
		data_queue.push(data);
	}

	// Ожидаем завершения всех задач
	data_queue.shutdownQueue();
	for (auto& t : pool) {
		t.join();
	}

	// Суммируем результаты
	for (float res : partial_results) {
		total_result += res;
	}

	return total_result;
}

//
//Standart
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

float not_parallel(float a, float b, std::string& expr)
{
	if (a > b) std::swap(a, b);
	float res{}, step = (b - a)/rectangle_cnt, cur_pos = a, heigh, x;

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

