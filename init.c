/*
 * Hocoslamfy, initialisation code file
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
#include <stddef.h>

#include "SDL.h"
#include "SDL_image.h"

#include "main.h"
#include "init.h"
#include "audio.h"
#include "platform.h"
#include "title.h"

static const char* BackgroundImageNames[BG_LAYER_COUNT] = {
	"Sky.png",
	"Mountains.png",
	"Clouds3.png",
	"Clouds2.png",
	"Clouds1.png",
	"Grass3.png",
	"Grass2.png",
	"Grass1.png"
};

static const char* TitleScreenFrameNames[TITLE_FRAME_COUNT] = {
	"TitleHeader1.png",
	"TitleHeader2.png",
	"TitleHeader3.png",
	"TitleHeader4.png",
	"TitleHeader5.png",
	"TitleHeader6.png",
	"TitleHeader7.png",
	"TitleHeader8.png"
};

static const char* FullPath(const char* Path)
{
	static char path[256];
	snprintf(path, sizeof path, DATA_PATH "%s", Path);
	return path;
}

static SDL_Surface* LoadImage(const char* Path)
{
	return IMG_Load(FullPath(Path));
}

static bool CheckImage(bool* Continue, bool* Error, const SDL_Surface* Image, const char* Name)
{
	if (Image == NULL)
	{
		*Continue = false;  *Error = true;
		printf("%s: LoadImage failed: %s\n", Name, IMG_GetError());
		return false;
	}
	else
	{
		printf("Successfully loaded image %s\n", Name);
		return true;
	}
}

static SDL_Texture* LoadTexture(bool* Continue, bool* Error, const char* Name)
{
	SDL_Texture* Texture = IMG_LoadTexture(Renderer, FullPath(Name));
	if (Texture == NULL)
	{
		*Continue = false;  *Error = true;
		printf("%s: LoadTexture failed: %s\n", Name, IMG_GetError());
	}
	else
	{
		printf("Successfully loaded texture %s\n", Name);
	}
	return Texture;
}

void Initialize(bool* Continue, bool* Error)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
		*Continue = false;  *Error = true;
		printf("SDL initialisation failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	} else printf("SDL initialisation succeeded\n");

	if (IMG_Init(IMG_INIT_PNG) == 0)
	{
		*Continue = false;  *Error = true;
		printf("IMG initialisation failed: %s\n", IMG_GetError());
		return;
	} else printf("IMG initialisation succeeded\n");

	Window = SDL_CreateWindow("hocoslamfy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_HIDDEN);
	if (Window == NULL)
	{
		*Continue = false;  *Error = true;
		printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("SDL_CreateWindow succeeded\n");

	SDL_Surface* WindowIcon = LoadImage("hocoslamfy.png");
	if (!CheckImage(Continue, Error, WindowIcon, "hocoslamfy.png"))
		return;
	SDL_SetWindowIcon(Window, WindowIcon);
	SDL_FreeSurface(WindowIcon);

	Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if (Renderer == NULL)
	{
		*Continue = false;  *Error = true;
		printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("SDL_CreateRenderer succeeded\n");

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(Renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetWindowFullscreen(Window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	SDL_ShowCursor(0);
	SDL_ShowWindow(Window);

	Screen = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
	if (Screen == NULL)
	{
		*Continue = false;  *Error = true;
		printf("SDL_CreateTexture failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("SDL_CreateTexture succeeded\n");

	uint32_t i;
	for (i = 0; i < BG_LAYER_COUNT; i++)
	{
		if ((BackgroundImages[i] = LoadTexture(Continue, Error, BackgroundImageNames[i])) == NULL)
			return;
	}

	for (i = 0; i < TITLE_FRAME_COUNT; i++)
	{
		if ((TitleScreenFrames[i] = LoadTexture(Continue, Error, TitleScreenFrameNames[i])) == NULL)
			return;
	}

	if ((CharacterFrames = LoadTexture(Continue, Error, "Bee.png")) == NULL)
		return;
	if ((CollisionImage = LoadTexture(Continue, Error, "Crash.png")) == NULL)
		return;
	if ((ColumnImage = LoadTexture(Continue, Error, "Bamboo.png")) == NULL)
		return;
	if ((GameOverFrame = LoadTexture(Continue, Error, "GameOverHeader.png")) == NULL)
		return;

	InitializePlatform();
	if (!InitializeAudio())
	{
		*Continue = false;  *Error = true;
		return;
	}
	else
		StartBGM();

	// Title screen. (-> title.c)
	ToTitleScreen();
}

void Finalize()
{
	uint32_t i;
	StopBGM();
	FinalizeAudio();
	if (Screen != NULL)
	{
		SDL_DestroyTexture(Screen);
		Screen = NULL;
	}
	for (i = 0; i < BG_LAYER_COUNT; i++)
	{
		if (BackgroundImages[i] != NULL)
		{
			SDL_DestroyTexture(BackgroundImages[i]);
			BackgroundImages[i] = NULL;
		}
	}
	for (i = 0; i < TITLE_FRAME_COUNT; i++)
	{
		if (TitleScreenFrames[i] != NULL)
		{
			SDL_DestroyTexture(TitleScreenFrames[i]);
			TitleScreenFrames[i] = NULL;
		}
	}
	if (CharacterFrames != NULL)
	{
		SDL_DestroyTexture(CharacterFrames);
		CharacterFrames = NULL;
	}
	if (ColumnImage != NULL)
	{
		SDL_DestroyTexture(ColumnImage);
		ColumnImage = NULL;
	}
	if (CollisionImage != NULL)
	{
		SDL_DestroyTexture(CollisionImage);
		CollisionImage = NULL;
	}
	if (GameOverFrame != NULL)
	{
		SDL_DestroyTexture(GameOverFrame);
		GameOverFrame = NULL;
	}
	if (Renderer != NULL)
	{
		SDL_DestroyRenderer(Renderer);
		Renderer = NULL;
	}
	if (Window != NULL)
	{
		SDL_DestroyWindow(Window);
		Window = NULL;
	}
	IMG_Quit();
	SDL_Quit();
}
