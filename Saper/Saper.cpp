#include <SFML/Graphics.hpp>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <vector>

using namespace sf;
using namespace std;

// Настройки игры
int board_height = 15; // от 2 до 32 (опираясь на монитор 1920x1080)
int board_width = 15; // от 2 (10 для комфортной игры) до 60 (опираясь на монитор 1920x1080)
int amount_of_mines = (board_height * board_width) / 7; // 10 - лёгкий, 7 - средний, 5 - сложный

// Пути к файлам текстур и размер спрайтов
string tiles_path = "tiles.jpg";
string buttons_path = "buttons.png";
int sprite_width = 32;

// Поля
int** visual_board = new int* [board_height];
int** logic_board = new int* [board_height];

// Хранение предыдущих ходов
int turn_number = 0;
vector<int**> turns = { visual_board };

// Глобальные переменные
int offsets[8][2] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, -1}, {-1, 1}, {1, -1} };
int arrows[2];
int amount_of_mines_left = amount_of_mines;
bool is_first_click = true;
bool is_exploded = false;
bool is_solved = false;
string solving_status = "not started";

// Запуск окна
RenderWindow app(VideoMode((board_width)*sprite_width, (board_height + 1)* sprite_width), "(" + solving_status + ") ", Style::Titlebar | Style::Close);

// Функции для дебага
void print_matrix(int** matrix) {
	for (int i = 0; i < board_height; i++) {
		for (int j = 0; j < board_width; j++) {
			cout << setw(2) << matrix[i][j] << "  ";
		}
		cout << endl;
	}
}

void check_win() {
	is_solved = true;
	amount_of_mines_left = amount_of_mines;
	for (size_t i = 0; i < board_height; ++i) {
		for (size_t j = 0; j < board_width; ++j) {
			if (visual_board[i][j] == 11) {
				amount_of_mines_left--;
			}
			if ((turns.back()[i][j] != logic_board[i][j] and (turns.back()[i][j] != 11 or logic_board[i][j] != 9)) or turns.back()[i][j] == 10) {
				is_solved = false;
			}
		}
	}
	if (is_solved and !is_exploded) {
		solving_status = "solved";
	}
	app.setTitle("(" + solving_status + ")" + " (" + to_string(turn_number + 1) + " / " + to_string(turns.size()) + ")" + " (" + to_string(amount_of_mines_left) + ")");
}

void draw_board(Sprite tiles) {
	for (int i = 0; i < board_height; i++) {
		for (int j = 0; j < board_width; j++) {
			tiles.setTextureRect(IntRect(visual_board[i][j] * sprite_width, 0, sprite_width, sprite_width));
			tiles.setPosition(j * sprite_width, i * sprite_width);
			app.draw(tiles);
		}
	}
}

void draw_hotbar(Sprite hotbar) {
	// Отрисовка кнопок 
	// Кнопка "Reset"
	hotbar.setTextureRect(IntRect(0, 0, sprite_width * 2, sprite_width));
	hotbar.setPosition(0, sprite_width * (board_height));
	app.draw(hotbar);
	// Стрелки
	hotbar.setTextureRect(IntRect(3 * sprite_width, 0, sprite_width, sprite_width));
	hotbar.setPosition(arrows[0] * sprite_width, sprite_width * (board_height));
	app.draw(hotbar);
	hotbar.setTextureRect(IntRect(4 * sprite_width, 0, sprite_width, sprite_width));
	hotbar.setPosition(arrows[1] * sprite_width, sprite_width * (board_height));
	app.draw(hotbar);
	// Кнопка "Solve"
	hotbar.setTextureRect(IntRect(5 * sprite_width, 0, sprite_width * 2, sprite_width));
	hotbar.setPosition((board_width - 2) * sprite_width, sprite_width * (board_height));
	app.draw(hotbar);
	// Заполнение пустот
	for (int i = 2; i < board_width - 2; i++) {
		if (arrows[0] != i and arrows[1] != i) {
			hotbar.setTextureRect(IntRect(2 * sprite_width, 0, sprite_width, sprite_width));
			hotbar.setPosition(i * sprite_width, sprite_width * (board_height));
			app.draw(hotbar);
		}
	}
}

void draw_empty_hotbar(Sprite hotbar) {
	for (int i = 0; i < board_width; i++) {
		hotbar.setTextureRect(IntRect(7 * sprite_width, 0, sprite_width, sprite_width));
		hotbar.setPosition(i * sprite_width, sprite_width * (board_height));
		app.draw(hotbar);
	}
}

bool is_valid_cord(int x, int y) {
	if (x >= board_height or y >= board_width or x < 0 or y < 0) return false;
	return true;
}

void reveal_empty_cells(int x, int y, int** board) {
	if (!is_valid_cord(x, y) or board[x][y] != 10) {
		return;
	}

	if (logic_board[x][y] == 0) {
		board[x][y] = 0;
		for (int k = 0; k < 8; k++) {
			int new_x = x + offsets[k][0], new_y = y + offsets[k][1];
			if (is_valid_cord(new_x, new_y) and logic_board[new_x][new_y] != 0 and board[new_x][new_y] == 10) {
				board[new_x][new_y] = logic_board[new_x][new_y];
			}
		}
		for (int i = 0; i < 8; i++) {
			reveal_empty_cells(x + offsets[i][0], y + offsets[i][1], board);
		}
	}
}

void fill_board(int** board) {
	for (int i = 0; i < board_height; i++) {
		board[i] = new int[board_width];
	}

	for (int i = 0; i < board_height; i++) {
		for (int j = 0; j < board_width; j++) {
			board[i][j] = 10;
		}
	}
}

void place_mines(int x, int y) {
	// Маркирование клеток рядом с первой нажатой на отсутствие мин
	for (int i = 0; i < 8; i++) {
		if (is_valid_cord(x + offsets[i][0], y + offsets[i][1])) {
			logic_board[x + offsets[i][0]][y + offsets[i][1]] = -1;
		}
	}

	// Процедура расстановки
	int mines_placed = 0;
	while (mines_placed < amount_of_mines) {
		int cell_x = rand() % board_height, cell_y = rand() % board_width;
		if (!is_valid_cord(cell_x, cell_y) or logic_board[cell_x][cell_y] == 9 or logic_board[cell_x][cell_y] == -1 or (cell_x == x and cell_y == y)) {
			continue;
		}
		else {
			logic_board[cell_x][cell_y] = 9;
			mines_placed++;
		}
	}
}

void place_numbers() {
	for (int i = 0; i < board_height; i++) {
		for (int j = 0; j < board_width; j++) {
			if (logic_board[i][j] == 9 or turns.back()[i][j] == 0) continue;

			int n = 0;

			for (int k = 0; k < 8; k++)
			{
				int ni = i + offsets[k][0];
				int nj = j + offsets[k][1];

				if (is_valid_cord(ni, nj) and logic_board[ni][nj] == 9)
					n++;
			}

			logic_board[i][j] = n;
		}
	}
}

void initialize_game(int x, int y) {
	// Отключение механизма первого клика
	is_first_click = false;
	
	// Заполнение логического поля
	for (int i = 0; i < board_height; i++) {
		logic_board[i] = new int[board_width];
	}
	for (int i = 0; i < board_height; i++) {
		for (int j = 0; j < board_width; j++) {
			logic_board[i][j] = 0;
		}
	}

	// Расстановка мин
	place_mines(x, y);

	// Создание поля следующего хода
	int** second_board = new int* [board_height];
	fill_board(second_board);
	turns.push_back(second_board);

	// Изменение показываемого поля
	turn_number++;
	visual_board = second_board;

	// Расстановка цифр
	place_numbers();

	// Открытие пустых клеток
	reveal_empty_cells(x, y, visual_board);
}

void open_cell(int x, int y, int** board) {
	if (logic_board[x][y] == 9) {
		board[x][y] = logic_board[x][y];
		is_exploded = true;
		solving_status = "lose";
	}
	else if (logic_board[x][y] == 0) {
		reveal_empty_cells(x, y, board);
	}
	else {
		board[x][y] = logic_board[x][y];
	}
	check_win();
}

void reset_game() {
	// Сброс переменной номера шага
	turn_number = 0;

	// Удаление предыдущих ходов и очищение памяти
	for (int i = 1; i < turns.size(); i++) {
		for (int j = 0; j < board_height; j++) {
			delete turns[i][j];
		}
		delete turns[i];
	}
	turns.erase(turns.begin() + 1, turns.end());

	for (int i = 0; i < board_height; i++) {
		delete logic_board[i];
	}

	visual_board = turns[0];
	
	// Сбрасываем флаги
	is_first_click = true;
	is_exploded = false;
	is_solved = false;

	// Меняем название окна
	solving_status = "not started";
	app.setTitle("(" + solving_status + ")");
}

void place_flags(int** board) {
	for (int m = 0; m < board_height; m++) {
		for (int n = 0; n < board_width; n++) {
			if (board[m][n] > 0 and board[m][n] < 9) {
				int number = board[m][n];
				int count = 0;
				int fields[8][2] = {};
				for (int k = 0; k < 8; k++) {
					int new_m = m + offsets[k][0];
					int new_n = n + offsets[k][1];
					if (is_valid_cord(new_m, new_n) and (board[new_m][new_n] == 10 or board[new_m][new_n] == 11)) {
						fields[count][0] = new_m;
						fields[count][1] = new_n;
						count++;
					}
					if (count > number) {
						break;
					}
				}
				if (count == number) {
					for (int k = 0; k < number; k++) {
						board[fields[k][0]][fields[k][1]] = 11;
					}
				}
			}
		}
	}
}

void clear_cells(int** board) {
	for (int m = 0; m < board_height; m++) {
		for (int n = 0; n < board_width; n++) {
			if (board[m][n] > 0 and board[m][n] < 9) {
				int number = board[m][n];
				int amount_of_cells = 0, count = 0;
				int fields[8][2] = {};
				for (int k = 0; k < 8; k++) {
					int new_m = m + offsets[k][0];
					int new_n = n + offsets[k][1];
					if (!is_valid_cord(new_m, new_n)) continue;
					if (board[new_m][new_n] == 11) {
						count++;
					}
					if (board[new_m][new_n] == 10) {
						fields[amount_of_cells][0] = new_m;
						fields[amount_of_cells][1] = new_n;
						amount_of_cells++;
					}
				}
				if (count == number) {
					for (int k = 0; k < amount_of_cells; k++) {
						open_cell(fields[k][0], fields[k][1], board);
					}
				}
			}
		}
	}
}

void solve_uncertainty(int** board) {
	// Копируем текущее поле в отдельную матрицу 
	int** current_board = new int* [board_height];
	for (int i = 0; i < board_height; i++) {
		current_board[i] = new int[board_width];
		for (int j = 0; j < board_width; j++) {
			current_board[i][j] = board[i][j];
		}
	}

	// Проходим по полю и определяем "шансы" мин в закртых полях
	int closed_cells = 0, flags = 0, mines = amount_of_mines;
	bool is_edited = false;
	for (int m = 0; m < board_height; m++) {
		for (int n = 0; n < board_width; n++) {
			if (board[m][n] == 10) closed_cells++;
			else if (current_board[m][n] == 11) flags++;
			else if (current_board[m][n] > 0 and current_board[m][n] < 9) {
				int number = current_board[m][n];
				for (int k = 0; k < 8; k++) {
					int new_m = m + offsets[k][0];
					int new_n = n + offsets[k][1];
					if (!is_valid_cord(new_m, new_n)) continue;
					if (current_board[new_m][new_n] == 11) {
						number--;
					}
					else if (current_board[new_m][new_n] == 10) {
						current_board[new_m][new_n] = 12;
					}
				}
				if (number == 0) continue;
				for (int k = 0; k < 8; k++) {
					int new_m = m + offsets[k][0];
					int new_n = n + offsets[k][1];
					if (!is_valid_cord(new_m, new_n)) continue;
					if (current_board[new_m][new_n] == 12) {
						current_board[new_m][new_n] = 0;
						current_board[new_m][new_n] -= number;
						is_edited = true;
					}
					else if (current_board[new_m][new_n] <= -1) {
						current_board[new_m][new_n] -= number;
					}
				}
			}
		}
	}
	// Отловка случая, когда кол-во оставшихся мин совпадает с кол-вом оставшихся закрытых клеток
	if (closed_cells == (mines - flags)) {
		for (int m = 0; m < board_height; m++) {
			for (int n = 0; n < board_width; n++) {
				if (board[m][n] == 10) {
					board[m][n] = 11;
				}
			}
		}
	}
	// Отловка случая, когда все мины расставлены, но остались закрытые поля
	else if (mines == flags) {
		for (int m = 0; m < board_height; m++) {
			for (int n = 0; n < board_width; n++) {
				if (board[m][n] == 10) {
					open_cell(m, n, board);
				}
			}
		}
	}
	// Отловка случая, когда не все мины расставлены, но остались закрытые поля, до которых нет досягаемости
	else if (!is_edited and (mines - flags) < closed_cells) {
		for (int m = 0; m < board_height; m++) {
			for (int n = 0; n < board_width; n++) {
				if (board[m][n] == 10) {
					open_cell(m, n, board);
					goto end;
				}
			}
		}
	}
	// Все остальные случаи
	else {
		is_edited = false;
		int min_x = -1, min_y = -1, min_f = 0;
		for (int m = 0; m < board_height; m++) {
			for (int n = 0; n < board_width; n++) {
				if (current_board[m][n] < min_f) {
					min_f = current_board[m][n];
					min_x = m;
					min_y = n;
				}
				// Открытие поля с опасностью "1" если это возможно
				if (current_board[m][n] == -1) {
					open_cell(m, n, board);
					goto end;
				}
			}
		}
		// Пометка флажком самого "вероятного" поля с миной 
		if (!is_edited and min_x != -1) {
			board[min_x][min_y] = 11;
		}
	}

	end:
	// Очистка памяти
	for (int i = 0; i < board_height; i++) {
		delete current_board[i];
	}
	delete current_board;

	check_win();
}

bool is_matrix_equials(int** matrix_1, int** matrix_2) {
	for (size_t i = 0; i < board_height; ++i) {
		for (size_t j = 0; j < board_width; ++j) {
			if (matrix_1[i][j] != matrix_2[i][j]) {
				return false;
			}
		}
	}

	return true;
}

void solve(int** board) {
	while (!is_solved and !is_exploded) {
		// Копируем текущее поле в отдельную матрицу
		int** turn = new int* [board_height];
		for (int i = 0; i < board_height; i++) {
			turn[i] = new int[board_width];
			for (int j = 0; j < board_width; j++) {
				turn[i][j] = turns.back()[i][j];
			}
		}

		// Расставляем флаги
		place_flags(turn);

		// Сохраняем состоние
		int** current_state = new int* [board_height];
		for (int i = 0; i < board_height; ++i) {
			current_state[i] = new int[board_width];
			copy(turn[i], turn[i] + board_width, current_state[i]);
		}
		turns.push_back(current_state);

		// Проверка на проигрыш
		if (is_exploded) {
			turns.erase(turns.end(), turns.end());
			break;
		}

		// Открываем возможные поля
		clear_cells(turn);

		// Сохраняем состоние
		current_state = new int* [board_height];
		for (int i = 0; i < board_height; ++i) {
			current_state[i] = new int[board_width];
			copy(turn[i], turn[i] + board_width, current_state[i]);
		}
		turns.push_back(current_state);

		// Проверка на проигрыш
		if (is_exploded) {
			turns.erase(turns.end(), turns.end());
			break;
		}

		// Проверка на неопределённость
		if (turns.size() >= 4 and is_matrix_equials(turns[turns.size() - 3], turns.back())) {
			solving_status = "uncertainty";
			is_solved = false;
			turns.erase(turns.end() - 2, turns.end());
			if (is_matrix_equials(turns[turns.size() - 1], turns.back())) {
				turns.erase(turns.end(), turns.end());
			}
			solve_uncertainty(turn);

			// Сохраняем состоние
			current_state = new int* [board_height];
			for (int i = 0; i < board_height; ++i) {
				current_state[i] = new int[board_width];
				copy(turn[i], turn[i] + board_width, current_state[i]);
			}
			turns.push_back(current_state);
		}

		// Проверка на проигрыш
		if (is_exploded) {
			turns.erase(turns.end(), turns.end());
			break;
		}

		// Очитка памяти
		for (int i = 0; i < board_height; i++) {
			delete turn[i];
		}
		delete turn;

		check_win();
	}
}

int main() {
	// Русификтор и сид для рандома
	setlocale(LC_ALL, "Russian");
	srand(time(0));

	// Создание и заполнение изначального полей
	fill_board(visual_board);

	// Вычисление позиции стрелок
	arrows[0] = (board_width % 2 == 0) ? (board_width / 2 - 1) : (board_width / 2 - 1);
	arrows[1] = (board_width % 2 == 0) ? (board_width / 2) : (board_width / 2 + 1);

	// Подгрузка текстур
	Texture board;
	board.loadFromFile(tiles_path);
	Sprite tiles(board);
	Texture buttons;
	buttons.loadFromFile(buttons_path);
	Sprite hotbar = Sprite(buttons);

	// Запуск окна с игрой
	while (app.isOpen())
	{
		Vector2i position = Mouse::getPosition(app);
		int x = position.y / sprite_width;
		int y = position.x / sprite_width;

		Event e;

		while (app.pollEvent(e))
		{
			if (e.type == Event::Closed)
				app.close();
			else if (e.type == Event::MouseButtonReleased and e.key.code == Mouse::Left) {
				if (is_valid_cord(x, y) and !is_exploded and !is_solved and (turn_number == turns.size() - 1 or is_first_click)) {
					if (is_first_click) {
						// Инициализируем игру
						initialize_game(x, y);

						// Меняем название окна
						solving_status = "not solved";
					}
					else {
						open_cell(x, y, visual_board);
					}

					check_win();
					app.setTitle("(" + solving_status + ")" + " (" + to_string(turn_number + 1) + " / " + to_string(turns.size()) + ")" + " (" + to_string(amount_of_mines_left) + ")");
				}
				else if (x == board_height and (y >= 0 and y <= 1) and !is_first_click) {
					reset_game();
				}
				else if (x == board_height and (y >= (board_width - 2) and y <= board_width - 1) and !is_first_click and !is_solved and !is_exploded and turn_number == turns.size() - 1) {
					solve(visual_board);
					visual_board = turns[turn_number];
				}
				else if (x = board_height and (y == arrows[0] or y == arrows[1]) and !is_first_click and (is_solved or solving_status == "uncertainty" or solving_status == "lose")) {
					if (y == arrows[0] and turn_number > 0) {
						turn_number--;
						visual_board = turns[turn_number];
					}
					else if (y == arrows[1] and turn_number < turns.size() - 1) {
						turn_number++;
						visual_board = turns[turn_number];
					}
					check_win();
				}

			}
			else if (e.type == Event::MouseButtonReleased and e.key.code == Mouse::Right) {
				if (is_valid_cord(x, y) and !is_exploded and !is_solved and (visual_board[x][y] == 10 or visual_board[x][y] == 11) and turn_number == turns.size() - 1) {
					if (visual_board[x][y] == 11) {
						visual_board[x][y] = 10;
					}
					else {
						visual_board[x][y] = 11;
					}
				}
				check_win();
			}
		}
		app.clear(Color::White);

		// Отрисовка поля
		draw_board(tiles);

		// Отрисовка кнопок
		if (!is_first_click) {
			draw_hotbar(hotbar);
		}
		else {
			draw_empty_hotbar(hotbar);
		}

		app.display();
	}

	// Очистка памяти
	for (int i = 0; i < board_height; ++i) {
		delete[] visual_board[i];
		delete[] logic_board[i];
	}
	delete[] visual_board;
	delete[] logic_board;
	return 0;
}