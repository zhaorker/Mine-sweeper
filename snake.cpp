#include<cstdio>
#include<random>
#include"conio.hpp"
#include<cstdlib>
#include<chrono>
#ifdef _WIN32
#include<windows.h>
#endif
constexpr int LX=15;
constexpr int LY=15;
std::uniform_int_distribution<int>food_xrange(0,LX-1);
std::uniform_int_distribution<int>food_yrange(0,LY-1);
std::default_random_engine eng(time(0));

int position_x{1},position_y{1},foodx,foody,direction_x{0},direction_y{1},length{3};

int map[LY][LX]{0};

int move(){
	for(int (&x)[LX]:map){
		for(int &i:x){
			i-= (i>0 ? 1 : 0);
		}
	}
	(position_x+=(direction_x+LX))%=LX;
	(position_y+=(direction_y+LY))%=LY;
	if(map[position_y][position_x]){
		puts("game over");
		return 1;
	}
	if((position_x^foodx)|(position_y^foody)){
		map[position_y][position_x]=length;
		return 0;
	}else{
		++length;
		map[position_y][position_x]=length;
		do{
			foodx=food_xrange(eng);
			foody=food_yrange(eng);
		}while(map[foody][foodx]);
		return 0;
	}
}
int putall(){
	printf("\033[H");
	bool iffood=true;
	for(int y=LY-1;y>=0;--y){
		for(int x=0;x<LX;++x){
			if(iffood&& y==foody && x==foodx){
				printf("\033[101m  \033[0m");
				iffood=false;
			}else if(map[y][x]){
				if(position_x==x&&position_y==y)
					printf("\033[102;94m :\033[0m");
				else
					printf("\033[102m  \033[0m");
			}else{
				printf("  ");
			}
		}
		putchar('|');
		putchar('\n');
	}
	return 0;
}
int main(){
#ifdef _WIN32
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);
#endif
	int second_speed=5;
	Conioer io;
	printf("speed rate:\n############");
	fflush(stdout);
	while(1){
		switch(io.getch()){
			case 'a':if(second_speed<10){
				++second_speed;
				printf("\b\b  \b\b");
			}break;
			case 'd':if(second_speed>1){
				--second_speed;
				printf("##");
			}break;
			case ' ':goto start;
		}
		fflush(stdout);
	}
	start:
	map[1][1]=length;
	do{
		foodx=food_xrange(eng);
		foody=food_yrange(eng);
	}while(map[foody][foodx]);
	int nx=0,ny=1;
	// clock_t wait=second_speed*CLOCKS_PER_SEC/10;
	auto wait=std::chrono::milliseconds(second_speed*100);
	while(1){
		putall();
		// clock_t start=clock();
		auto start=std::chrono::steady_clock::now();
		while(std::chrono::steady_clock::now()-start<wait){
			if(io.kbhit())
				switch(io.getch()){
					case 'w':nx=0;ny=1;break;
					case 's':nx=0;ny=-1;break;
					case 'a':nx=-1;ny=0;break;
					case 'd':nx=1;ny=0;break;
					case 3:std::exit(-1);
				}
			}
		if(!(nx&&direction_x||ny&&direction_y)){
			direction_x=nx;
			direction_y=ny;
		}
		if(move()){
			return 0;
		}
	}
}