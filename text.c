/*
 * Hocoslamfy, text rendering code file
 * Copyright (C) 2013 Nebuleon Fumika <nebuleon@gcw-zero.com>
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

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "text.h"
#include "unifont.h"

struct StringCut {
	uint32_t Start;  // Starting character index of the cut, inclusive.
	uint32_t End;    // Ending character index of the cut, exclusive.
};

static uint32_t CutString(const char* String, const uint32_t MaxWidth,
	struct StringCut* Cuts, uint32_t CutsAllocated)
{
	uint32_t Cut = 0;
	uint32_t CutStart = 0, Cur = 0, CutWidth = 0;
	uint32_t LastSpace = -1;
	bool SpaceInCut = false;
	while (String[Cur] != '\0')
	{
		if (String[Cur] != '\n')
		{
			if (String[Cur] == ' ')
			{
				LastSpace = Cur;
				SpaceInCut = true;
			}
			CutWidth += _font_width[(uint8_t) String[Cur]];
		}

		if (String[Cur] == '\n' || CutWidth > MaxWidth)
		{
			if (Cut < CutsAllocated)
				Cuts[Cut].Start = CutStart;
			if (String[Cur] == '\n')
			{
				if (Cut < CutsAllocated)
					Cuts[Cut].End = Cur;
			}
			else if (CutWidth > MaxWidth)
			{
				if (SpaceInCut)
				{
					if (Cut < CutsAllocated)
						Cuts[Cut].End = LastSpace;
					Cur = LastSpace;
				}
				else
				{
					if (Cut < CutsAllocated)
						Cuts[Cut].End = Cur;
					Cur--; // Next iteration redoes this character
				}
			}
			CutStart = Cur + 1;
			CutWidth = 0;
			SpaceInCut = false;
			Cut++;
		}
		Cur++;
	}

	if (Cut < CutsAllocated)
	{
		Cuts[Cut].Start = CutStart;
		Cuts[Cut].End = Cur;
	}
	return Cut + 1;
}

uint32_t GetSectionRenderedWidth(const char* String, const uint32_t Start, const uint32_t End)
{
	uint32_t Result = 0, i;
	for (i = Start; i < End; i++)
		Result += _font_width[(uint8_t) String[i]];
	return Result;
}

void PrintString(SDL_Renderer *Renderer, const char* String,
	const SDL_Color* TextColor, const SDL_Rect* Dest,
	enum HorizontalAlignment HorizontalAlignment, enum VerticalAlignment VerticalAlignment)
{
	SDL_Color OldColor;
	SDL_GetRenderDrawColor(Renderer, &OldColor.r, &OldColor.g, &OldColor.b, &OldColor.a);
	SDL_SetRenderDrawColor(Renderer, TextColor->r, TextColor->g, TextColor->b, TextColor->a);

	struct StringCut* Cuts = malloc((Dest->h / _font_height) * sizeof(struct StringCut));
	uint32_t CutCount = CutString(String, Dest->w, Cuts, Dest->h / _font_height), Cut;
	if (CutCount > Dest->h / _font_height)
		CutCount = Dest->h / _font_height;

	for (Cut = 0; Cut < CutCount; Cut++)
	{
		uint32_t TextWidth = GetSectionRenderedWidth(String, Cuts[Cut].Start, Cuts[Cut].End);
		int LineX, LineY;
		switch (HorizontalAlignment)
		{
			case LEFT:   LineX = Dest->x;                             break;
			case CENTER: LineX = Dest->x + (Dest->w - TextWidth) / 2; break;
			case RIGHT:  LineX = (Dest->x + Dest->w) - TextWidth;     break;
			default:     LineX = 0; /* shouldn't happen */            break;
		}
		switch (VerticalAlignment)
		{
			case TOP:
				LineY = Dest->y + Cut * _font_height;
				break;
			case MIDDLE:
				LineY = Dest->y + (Dest->h - CutCount * _font_height) / 2 + Cut * _font_height;
				break;
			case BOTTOM:
				LineY = (Dest->y + Dest->h) - (CutCount - Cut) * _font_height;
				break;
			default:
				LineY = 0; /* shouldn't happen */
				break;
		}

		uint32_t Cur;
		for (Cur = Cuts[Cut].Start; Cur < Cuts[Cut].End; Cur++)
		{
			uint32_t glyph_offset = (uint32_t) String[Cur] * _font_height;
			uint32_t glyph_width = _font_width[(uint8_t) String[Cur]];
			uint32_t glyph_column, glyph_row;
			uint16_t current_halfword;

			for(glyph_row = 0; glyph_row < _font_height; glyph_row++, glyph_offset++)
			{
				current_halfword = _font_bits[glyph_offset];
				for (glyph_column = 0; glyph_column < glyph_width; glyph_column++)
				{
					if ((current_halfword >> (15 - glyph_column)) & 0x01)
						SDL_RenderDrawPoint(Renderer, LineX + glyph_column, LineY + glyph_row);
				}
			}

			LineX += glyph_width;
		}
	}

	free(Cuts);

	SDL_SetRenderDrawColor(Renderer, OldColor.r, OldColor.g, OldColor.b, OldColor.a);
}

uint32_t GetRenderedWidth(const char* str)
{
	struct StringCut* Cuts = malloc(sizeof(struct StringCut));
	uint32_t CutCount = CutString(str, UINT32_MAX, Cuts, 1);
	if (CutCount > 1)
	{
		Cuts = realloc(Cuts, CutCount * sizeof(struct StringCut));
		CutString(str, UINT32_MAX, Cuts, CutCount);
	}

	uint32_t Result = 0, LineWidth, Cut;
	for (Cut = 0; Cut < CutCount; Cut++)
	{
		LineWidth = 0;
		uint32_t Cur;
		for (Cur = Cuts[Cut].Start; Cur < Cuts[Cut].End; Cur++)
		{
			LineWidth += _font_width[(uint8_t) str[Cur]];
		}
		if (LineWidth > Result)
			Result = LineWidth;
	}

	free(Cuts);

	return Result;
}

uint32_t GetRenderedHeight(const char* str)
{
	return CutString(str, UINT32_MAX, NULL, 0) * _font_height;
}

void PrintStringOutline(SDL_Renderer* Renderer, const char* String,
	const SDL_Color* TextColor, const SDL_Color* OutlineColor, const SDL_Rect* Dest,
	enum HorizontalAlignment HorizontalAlignment, enum VerticalAlignment VerticalAlignment)
{
	SDL_Rect TempDest;
	TempDest.w = Dest->w - 2;
	TempDest.h = Dest->h - 2;
	int sx, sy;
	for (sx = 0; sx <= 2; sx++)
		for (sy = 0; sy <= 2; sy++)
			if (!(sx == 1 && sy == 1))
			{
				TempDest.x = Dest->x + sx;
				TempDest.y = Dest->y + sy;
				PrintString(Renderer, String, OutlineColor, &TempDest, HorizontalAlignment, VerticalAlignment);
			}
	TempDest.x = Dest->x + 1;
	TempDest.y = Dest->y + 1;
	PrintString(Renderer, String, TextColor, &TempDest, HorizontalAlignment, VerticalAlignment);
}

