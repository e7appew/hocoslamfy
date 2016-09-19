/*
 * Hocoslamfy, title screen code file
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
#include <stdlib.h>

#include "SDL.h"
#include "SDL_image.h"

#include "main.h"
#include "init.h"
#include "platform.h"
#include "game.h"
#include "title.h"
#include "bg.h"
#include "text.h"

static bool     WaitingForRelease = false;
static char*    WelcomeMessage    = NULL;

static uint32_t HeaderFrame       = 0;
static Uint32   HeaderFrameTime   = 0;

static const uint32_t HeaderFrameAnimation[TITLE_ANIMATION_FRAMES] = {
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, /* up */
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 2, 3, /* blink */
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 2, 3, /* blink */
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
	4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, /* down */
	4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5,
	4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 6, 7, /* blink */
	4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5,
	4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 6, 7, /* blink */
	4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5,
};

void TitleScreenGatherInput(bool* Continue)
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
			if (WelcomeMessage != NULL)
			{
				free(WelcomeMessage);
				WelcomeMessage = NULL;
			}
			return;
		}
		else if (IsExitGameEvent(&ev))
		{
			*Continue = false;
			if (WelcomeMessage != NULL)
			{
				free(WelcomeMessage);
				WelcomeMessage = NULL;
			}
			return;
		}
		else if (IsToggleFullscreenEvent(&ev))
		{
			ToggleFullscreen();
		}
	}
}

static void AnimationControl(Uint32 Milliseconds)
{
	Uint32 Remainder = Milliseconds;

	// Get rid of all the times the animation could have been fully
	// completed since the last frame displayed.
	Remainder = Remainder % (TITLE_FRAME_TIME * TITLE_ANIMATION_FRAMES);
	// If needed, advance the frame by however many steps are now
	// fully done.
	HeaderFrame = (HeaderFrame + (HeaderFrameTime + Remainder) / TITLE_FRAME_TIME) % TITLE_ANIMATION_FRAMES;
	// Then add milliseconds for the current frame.
	HeaderFrameTime = (HeaderFrameTime + Remainder) % TITLE_FRAME_TIME;
}

void TitleScreenDoLogic(bool* Continue, bool* Error, Uint32 Milliseconds)
{
	AnimationControl(Milliseconds);
	AdvanceBackground(Milliseconds);
}

void TitleScreenOutputFrame()
{
	SDL_SetRenderTarget(Renderer, Screen);
	SDL_RenderClear(Renderer);

	DrawBackground();

	int Width = 0;
	int Height = 0;
	SDL_QueryTexture(TitleScreenFrames[0], NULL, NULL, &Width, &Height);
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
	SDL_RenderCopy(Renderer, TitleScreenFrames[HeaderFrameAnimation[HeaderFrame]], &HeaderSourceRect, &HeaderDestRect);

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
	PrintStringOutline(Renderer, WelcomeMessage, &TextColor, &OutlineColor, &Dest, CENTER, MIDDLE);

	SDL_SetRenderTarget(Renderer, NULL);
	SDL_RenderClear(Renderer);
	SDL_RenderCopy(Renderer, Screen, NULL, NULL);
	SDL_RenderPresent(Renderer);
}

void ToTitleScreen(void)
{
	if (WelcomeMessage == NULL)
	{
		int Length = 2, NewLength;
		WelcomeMessage = malloc(Length);
		while ((NewLength = snprintf(WelcomeMessage, Length, "Press %s to play\n%s to toggle fullscreen\nor %s to exit\n\nIn-game:\n%s to rise\n%s to pause\n%s to exit", GetEnterGamePrompt(), GetToggleFullscreenPrompt(), GetExitGamePrompt(), GetBoostPrompt(), GetPausePrompt(), GetExitGamePrompt())) >= Length)
		{
			Length = NewLength + 1;
			WelcomeMessage = realloc(WelcomeMessage, Length);
		}
	}

	GatherInput = TitleScreenGatherInput;
	DoLogic     = TitleScreenDoLogic;
	OutputFrame = TitleScreenOutputFrame;
}
