/*
 * Hocoslamfy, score screen code file
 * Copyright (C) 2014 Nebuleon Fumika <nebuleon@gcw-zero.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdbool.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef DONT_USE_PWD
#include <pwd.h>
#endif

#include "SDL.h"

#include "main.h"
#include "init.h"
#include "platform.h"
#include "game.h"
#include "score.h"
#include "bg.h"
#include "text.h"
#include "audio.h"

static bool  WaitingForRelease = false;

static char* ScoreMessage      = NULL;

static const char* SavePath = ".hocoslamfy";
static const char* HighScoreFilePath = "highscore";

void ScoreGatherInput(bool* Continue)
{
	SDL_Event ev;

	while (SDL_PollEvent(&ev))
	{
		if (!WaitingForRelease && IsEnterGamePressingEvent(&ev))
			WaitingForRelease = true;
		else if (WaitingForRelease && IsEnterGameReleasingEvent(&ev))
		{
			WaitingForRelease = false;
			ToGame();
			if (ScoreMessage != NULL)
			{
				free(ScoreMessage);
				ScoreMessage = NULL;
			}
			return;
		}
		else if (IsExitGameEvent(&ev))
		{
			*Continue = false;
			if (ScoreMessage != NULL)
			{
				free(ScoreMessage);
				ScoreMessage = NULL;
			}
			return;
		}
		else if (IsToggleFullscreenEvent(&ev))
		{
			ToggleFullscreen();
		}
	}
}

void ScoreDoLogic(bool* Continue, bool* Error, Uint32 Milliseconds)
{
	AdvanceBackground(Milliseconds);
}

void ScoreOutputFrame()
{
	SDL_SetRenderTarget(Renderer, Screen);
	SDL_RenderClear(Renderer);

	DrawBackground();

	int Width = 0;
	int Height = 0;
	SDL_QueryTexture(GameOverFrame, NULL, NULL, &Width, &Height);
	SDL_Rect HeaderDestRect = {
		.x = (SCREEN_WIDTH - Width) / 2,
		.y = ((SCREEN_HEIGHT / 4) - Height) / 2,
		.w = Width,
		.h = Height
	};
	SDL_Rect HeaderSourceRect = {
		.x = 0,
		.y = 0,
		.w = Width,
		.h = Height
	};
	SDL_RenderCopy(Renderer, GameOverFrame, &HeaderSourceRect, &HeaderDestRect);

	SDL_Color TextColor = {
		.r = 255,
		.g = 255,
		.b = 255,
		.a = 255
	};
	SDL_Color OutlineColor = {
		.r = 0,
		.g = 0,
		.b = 0,
		.a = 255
	};
	SDL_Rect Dest = {
		.x = 0,
		.y = SCREEN_HEIGHT / 4,
		.w = SCREEN_WIDTH,
		.h = SCREEN_HEIGHT - (SCREEN_HEIGHT / 4)
	};
	PrintStringOutline(Renderer, ScoreMessage, &TextColor, &OutlineColor, &Dest, CENTER, MIDDLE);

	SDL_SetRenderTarget(Renderer, NULL);
	SDL_RenderClear(Renderer);
	SDL_RenderCopy(Renderer, Screen, NULL, NULL);
	SDL_RenderPresent(Renderer);
}

void ToScore(uint32_t Score, enum GameOverReason GameOverReason, uint32_t HighScore)
{
	if (ScoreMessage != NULL)
	{
		free(ScoreMessage);
		ScoreMessage = NULL;
	}
	int Length = 2, NewLength;
	ScoreMessage = malloc(Length);

	const char* GameOverReasonString = "";
	switch (GameOverReason)
	{
		case FIELD_BORDER_COLLISION:
			GameOverReasonString = "You flew too far away from the field";
			break;
		case RECTANGLE_COLLISION:
			GameOverReasonString = "You crashed into a bamboo shoot";
			break;
	}

	char HighScoreString[256];
	if (Score > HighScore)
	{
		snprintf(HighScoreString, 256, "NEW High Score: %" PRIu32, Score);
		PlaySFXHighScore();
	} else {
		snprintf(HighScoreString, 256, "High Score: %" PRIu32, HighScore);
	}

	while ((NewLength = snprintf(ScoreMessage, Length, "%s\n\nYour score was %" PRIu32 "\n\n%s\n\nPress %s to play again\n%s to toggle fullscreen\nor %s to exit", GameOverReasonString, Score, HighScoreString, GetEnterGamePrompt(), GetToggleFullscreenPrompt(), GetExitGamePrompt())) >= Length)
	{
		Length = NewLength + 1;
		ScoreMessage = realloc(ScoreMessage, Length);
	}

	GatherInput = ScoreGatherInput;
	DoLogic     = ScoreDoLogic;
	OutputFrame = ScoreOutputFrame;
}

int MkDir(char *path)
{
#ifndef DONT_USE_PWD
	return mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
#else
	return mkdir(path);
#endif
}

void SaveHighScore(uint32_t Score)
{
	char path[256];
#ifndef DONT_USE_PWD
	struct passwd *pw = getpwuid(getuid());
	
	snprintf(path, 256, "%s/%s", pw->pw_dir, SavePath);
	MkDir(path);
	
	snprintf(path, 256, "%s/%s/%s", pw->pw_dir, SavePath, HighScoreFilePath);
#else
	snprintf(path, 256, "%s", HighScoreFilePath);
#endif
	FILE *fp = fopen(path, "w");

	if (!fp)
	{
		fprintf(stderr, "%s: Unable to open file.\n", path);
		return;
	}

	fprintf(fp, "%" PRIu32, Score);
	fclose(fp);
}

void GetFileLine(char *str, uint32_t size, FILE *fp)
{
	int i = 0;
	for (i = 0; i < size - 1; i++)
	{
		int c = fgetc(fp);
		if (c == EOF || c == '\n')
		{
			str[i] = '\0';
			break;
		}
		str[i] = c;
	}
	str[size - 1] = '\0';
}

uint32_t GetHighScore()
{
	char path[256];
#ifndef DONT_USE_PWD
	struct passwd *pw = getpwuid(getuid());
	snprintf(path, 256, "%s/%s/%s", pw->pw_dir, SavePath, HighScoreFilePath);
#else
	snprintf(path, 256, "%s", HighScoreFilePath);
#endif

	FILE *fp = fopen(path, "r");

	if (!fp)
		return 0;

	char line[256];
	GetFileLine(line, 256, fp);
	fclose(fp);
	
	uint32_t hs = 0;
	if (sscanf(line, "%" SCNu32, &hs) != 1)
		return 0;

	return hs;
}

