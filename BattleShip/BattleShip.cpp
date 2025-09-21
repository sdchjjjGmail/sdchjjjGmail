#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <random>

using namespace std;

constexpr int N = 10;
enum class CellView { Unknown, Miss, Hit };           // 플레이어가 보는 맵
enum class AttackResult { Repeat, Miss, Hit, Sunk };  // 공격 결과

struct Coord {
    int x; // 0..9
    int y; // 0..9
};

struct Ship {
    int size;
    vector<Coord> cells;       // 선박이 차지하는 좌표들
    vector<bool> hits;         // 각 칸 피격 여부

    Ship(int sz = 0) : size(sz), hits(sz, false) {}

    bool occupies(int x, int y, int& idxOut) const {
        for (int i = 0; i < (int)cells.size(); ++i) {
            if (cells[i].x == x && cells[i].y == y) { idxOut = i; return true; }
        }
        return false;
    }
    bool isSunk() const {
        for (bool h : hits) if (!h) return false;
        return true;
    }
    void markHitIndex(int idx) {
        if (idx >= 0 && idx < (int)hits.size()) hits[idx] = true;
    }
};

class Board {
public:
    Board()
        : shipIndex(N, vector<int>(N, -1)),
        view(N, vector<CellView>(N, CellView::Unknown)) {}

    void placeShipsRandomly(const vector<int>& sizes) {
        ships.clear();
        for (int sz : sizes) {
            placeSingleShip(sz);
        }
    }

    AttackResult attack(int x, int y, string& msg) {
        // 이미 시도한 좌표?
        if (view[y][x] != CellView::Unknown) {
            msg = "이미 공격한 좌표입니다.";
            return AttackResult::Repeat;
        }

        int idx = shipIndex[y][x];
        if (idx == -1) {
            view[y][x] = CellView::Miss;
            msg = "실패(Miss).";
            return AttackResult::Miss;
        }
        else {
            view[y][x] = CellView::Hit;
            int cellIdx = -1;
            ships[idx].occupies(x, y, cellIdx);
            ships[idx].markHitIndex(cellIdx);
            if (ships[idx].isSunk()) {
                msg = "격침(Sunk)!";
                return AttackResult::Sunk;
            }
            else {
                msg = "명중(Hit)!";
                return AttackResult::Hit;
            }
        }
    }

    int remainingShips() const {
        int cnt = 0;
        for (auto& s : ships) if (!s.isSunk()) ++cnt;
        return cnt;
    }

    bool allSunk() const { return remainingShips() == 0; }

    void printPlayerView() const {
        // 헤더
        cout << "    ";
        for (int x = 0; x < N; ++x) cout << setw(2) << (x + 1) << ' ';
        cout << "\n";

        cout << "   +" << string(N * 3, '-') << "+\n";
        for (int y = 0; y < N; ++y) {
            cout << setw(2) << (y + 1) << " |";
            for (int x = 0; x < N; ++x) {
                char c = '.';
                if (view[y][x] == CellView::Miss) c = 'o';
                else if (view[y][x] == CellView::Hit) c = 'X';
                cout << ' ' << c << ' ';
            }
            cout << "|\n";
        }
        cout << "   +" << string(N * 3, '-') << "+\n";
        cout << "표시: . 미확인 / o 실패 / X 명중\n";
    }

    void printRevealAll() const {
        // 실제 적 함선 위치(S), 명중(X), 빗나감(o)
        cout << "=== 적 함선 실제 배치(패배 시 공개) ===\n";
        cout << "    ";
        for (int x = 0; x < N; ++x) cout << setw(2) << (x + 1) << ' ';
        cout << "\n";
        cout << "   +" << string(N * 3, '-') << "+\n";
        for (int y = 0; y < N; ++y) {
            cout << setw(2) << (y + 1) << " |";
            for (int x = 0; x < N; ++x) {
                char c = '.';
                int idx = shipIndex[y][x];
                if (idx >= 0) {
                    // 해당 위치가 명중되었는지 확인
                    int cellIdx = -1;
                    bool occ = ships[idx].occupies(x, y, cellIdx);
                    if (occ && ships[idx].hits[cellIdx]) c = 'X';
                    else c = 'S';
                }
                else if (view[y][x] == CellView::Miss) {
                    c = 'o';
                }
                cout << ' ' << c << ' ';
            }
            cout << "|\n";
        }
        cout << "   +" << string(N * 3, '-') << "+\n";
        cout << "표시: S 함선 / X 명중 / o 실패 / . 빈칸\n";
    }

private:
    vector<vector<int>> shipIndex;      // 각 칸에 어느 Ship(인덱스)가 있는지 (-1: 없음)
    vector<vector<CellView>> view;      // 플레이어가 보는 정보
    vector<Ship> ships;

    void placeSingleShip(int sz) {
        static random_device rd;
        static mt19937 gen(rd());
        uniform_int_distribution<int> dirDist(0, 1); // 0: 가로, 1: 세로

        while (true) {
            int dir = dirDist(gen);
            int dx = (dir == 0) ? 1 : 0;
            int dy = (dir == 1) ? 1 : 0;

            int maxX = N - (dx ? sz : 1);
            int maxY = N - (dy ? sz : 1);

            uniform_int_distribution<int> xDist(0, maxX);
            uniform_int_distribution<int> yDist(0, maxY);

            int sx = xDist(gen);
            int sy = yDist(gen);

            if (canPlace(sx, sy, dx, dy, sz)) {
                Ship s(sz);
                for (int k = 0; k < sz; ++k) {
                    int x = sx + dx * k;
                    int y = sy + dy * k;
                    s.cells.push_back({ x, y });
                }
                s.hits.assign(sz, false);
                int idx = (int)ships.size();
                ships.push_back(s);
                for (int k = 0; k < sz; ++k) {
                    int x = s.cells[k].x;
                    int y = s.cells[k].y;
                    shipIndex[y][x] = idx;
                }
                return;
            }
            // 실패 시 루프 반복하여 다른 위치 시도
        }
    }

    bool canPlace(int sx, int sy, int dx, int dy, int sz) const {
        for (int k = 0; k < sz; ++k) {
            int x = sx + dx * k;
            int y = sy + dy * k;
            if (x < 0 || x >= N || y < 0 || y >= N) return false;
            if (shipIndex[y][x] != -1) return false; // 겹침 금지
        }
        return true;
    }
};

class Game {
public:
    Game()
        : maxShots(50), shotsLeft(50) {
        vector<int> sizes = { 5, 4, 3, 2 };
        board.placeShipsRandomly(sizes);
    }

    void run() {
        cout << "===== 1인 배틀쉽 =====\n";
        cout << "맵 크기: 10 x 10\n";
        cout << "적 함선: 4척 (크기 5,4,3,2)\n";
        cout << "제한 공격 횟수: " << maxShots << "회\n";
        cout << "입력 형식: x y  (각 1~10)\n";
        cout << "----------------------\n\n";

        while (shotsLeft > 0 && !board.allSunk()) {
            board.printPlayerView();
            cout << "남은 공격 횟수: " << shotsLeft
                << " | 남은 적 함선 수: " << board.remainingShips() << "\n";

            int x, y;
            if (!readCoord(x, y)) {
                cout << "입력이 올바르지 않습니다. 예) 3 7\n";
                continue;
            }

            // 내부 인덱스(0~9)로 변환
            x -= 1; y -= 1;

            if (x < 0 || x >= N || y < 0 || y >= N) {
                cout << "좌표 범위를 벗어났습니다. 1~10 사이로 입력하세요.\n";
                continue;
            }

            string msg;
            AttackResult res = board.attack(x, y, msg);
            if (res == AttackResult::Repeat) {
                cout << msg << " 다른 좌표를 입력하세요.\n\n";
                continue; // 소모 없음
            }
            else {
                --shotsLeft;
                cout << msg << "\n\n";
            }
        }

        // 종료 처리
        if (board.allSunk()) {
            cout << "🎉 모든 적 함선을 격침했습니다! 승리!\n";
        }
        else {
            cout << "😢 제한 횟수 내에 격침하지 못했습니다. 패배...\n";
            board.printRevealAll();
        }
        cout << "게임 종료.\n";
    }

private:
    Board board;
    const int maxShots;
    int shotsLeft;

    bool readCoord(int& xOut, int& yOut) {
        cout << "공격 좌표 입력 (x y): ";
        string line;
        if (!std::getline(cin, line)) return false;
        if (line.empty()) return false;
        istringstream iss(line);
        if (!(iss >> xOut >> yOut)) return false;
        return true;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Game game;
    game.run();
    return 0;
}
