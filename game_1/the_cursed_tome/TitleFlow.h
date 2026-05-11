#pragma once

#include <stdint.h>

class TitleFlow
{
public:
	void Init();
	void Draw();
	void Tick();

	void TickEnteringLevel();
	void DrawEnteringLevel();	

	void TickGameOver();
	void DrawGameOver();	

	void TickMissionComplete();
	void DrawMissionComplete();
	
	void ResetTimer();
	
	void FadeOut();
	
private:
	union
	{
		uint8_t selection;
		uint16_t timer;
		uint16_t fizzleFade;
	};
	
};


