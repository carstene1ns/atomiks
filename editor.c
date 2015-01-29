/*
 * This file is part of the Atomiks project
 * Copyright (C) Mateusz Viste 2013, 2014, 2015
 *
 * Level editor for Atomiks
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "atomcore.h"
#include "data.h"
#include "gz.h"

void savelevel(struct atomixgame *game, int level) {
  char levelfile[64];
  FILE *fd;
  int x, y;
  sprintf(levelfile, "lev/lev%04d.dat", level);
  fd = fopen(levelfile, "wb");
  if (fd == NULL) return;
  /* write initial playfield */
  for (y = 0; y < 16; y++) {
    for (x = 0; x < 16; x++) {
      fputc(game->field[x][y], fd);
    }
  }
  /* write solution */
  for (y = 0; y < 16; y++) {
    for (x = 0; x < 16; x++) {
      fputc(game->solution[x][y], fd);
    }
  }
  /* write the timer */
  fputc((game->duration >> 8) & 0xFF, fd);
  fputc(game->duration & 0xFF, fd);
  /* write level descriptions */
  for (x = 0; x < 15; x++) fputc(game->level_desc_line1[x], fd);
  for (x = 0; x < 15; x++) fputc(game->level_desc_line2[x], fd);
  /* write the cursor type */
  fputc(game->cursortype, fd);
  /* write the bg id */
  fputc(game->bg, fd);
  fclose(fd);
}


/* loads a gziped bmp image from memory and returns a surface */
static SDL_Surface *loadgzbmp(unsigned char *memgz, long memgzlen) {
  SDL_RWops *rwop;
  SDL_Surface *surface;
  unsigned char *rawimage;
  long rawimagelen;
  if (isGz(memgz, memgzlen) == 0) return(NULL);
  rawimage = ungz(memgz, memgzlen, &rawimagelen);
  rwop = SDL_RWFromMem(rawimage, rawimagelen);
  surface = SDL_LoadBMP_RW(rwop, 0);
  SDL_FreeRW(rwop);
  free(rawimage);
  return(surface);
}


static void loadGraphic(SDL_Surface **surface, void *memptr, int memlen) {
  *surface = loadgzbmp(memptr, memlen);
}


void loadSpriteSheet(SDL_Surface **surface, int width, int height, int itemcount, void *memptr, int memlen) {
  SDL_Surface *spritesheet, *item;
  SDL_Rect rect;
  int i;
  loadGraphic(&spritesheet, memptr, memlen);
  item = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, 0xFF000000L, 0x00FF0000L, 0x0000FF00L, 0x000000FFL);
  for (i = 0; i < itemcount; i++) {
    rect.x = i * width;
    rect.y = 0;
    rect.w = width;
    rect.h = height;
    SDL_FillRect(item, NULL, 0);
    /* SDL_SetAlpha(spritesheet, 0, spritesheet->format->alpha); */ /* disable alpha blending to copy pixels as-is */
    SDL_BlitSurface(spritesheet, &rect, item, NULL);
    surface[i] = SDL_CreateRGBSurface(SDL_SWSURFACE, width << 1, height << 1, 32, 0xFF000000L, 0x00FF0000L, 0x0000FF00L, 0x000000FFL);
    /* scale2x(item, surface[i]); */
  }
  SDL_FreeSurface(item);
  SDL_FreeSurface(spritesheet);
}



int main(int argc, char **argv) {
  int x, y, level, exitflag = 0;
  int viewmode = 0;  /* 0 = edit level / 1 = edit solution */
  int cursorx = 0, cursory = 0;
  int selectedline = 1, selectedchar = 0;
  int lastitem = 0;
  struct atomixgame *game;
  SDL_Surface *screen = NULL;
  SDL_Surface *bg[3], *wall[19], *empty, *atom[64], *tile, *cursor[3], *font[16], *font3[64];
  SDL_Rect rect;
  SDL_Event event;
  if (argc != 2) {
    puts("Usage: editor levelnum");
    return(1);
  }
  level = atoi(argv[1]);

  game = malloc(sizeof(struct atomixgame));
  if (game == NULL) {
    puts("Error: out of memory");
    return(2);
  }
  atomix_loadgame(game, level, ATOMIX_SRC_FILE, NULL);

  SDL_Init(SDL_INIT_VIDEO);
  if ((screen = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE | SDL_DOUBLEBUF)) == NULL) {
    puts("Error: unable to init screen!");
    return(1);
  }

  /* Set keyboard repeat rate */
  SDL_EnableKeyRepeat(100, 70);

  /* Preload all graphics */
  /* load the 'empty space' tile */
  loadSpriteSheet(&empty, 16, 16, 1, img_empty_bmp_gz, img_empty_bmp_gz_len);
  /* load backgrounds */
  loadSpriteSheet(&bg[0], 320, 240, 3, img_bg_bmp_gz, img_bg_bmp_gz_len);
  /* load atoms sprites */
  loadSpriteSheet(&atom[0], 16, 16, 49, img_atoms_bmp_gz, img_atoms_bmp_gz_len);
  /* load walls sprites */
  loadSpriteSheet(&wall[0], 16, 16, 19, img_walls_bmp_gz, img_walls_bmp_gz_len);
  /* load cursor sprites */
  loadSpriteSheet(&cursor[0], 16, 16, 3, img_cursors_bmp_gz, img_cursors_bmp_gz_len);
  /* load timer font */
  loadSpriteSheet(&font[0], 14, 15, 11, img_font2_bmp_gz, img_font2_bmp_gz_len);
  /* load generic font */
  loadSpriteSheet(&font3[0], 7, 7, 26, img_font3_bmp_gz, img_font3_bmp_gz_len);

  SDL_EnableUNICODE(1);

  for (;;) {
    SDL_BlitSurface(bg[game->bg], NULL, screen, NULL); /* draw the background */
    /* Draw the timer */
    rect.x = 520;
    rect.y = 20;
    rect.w = font[0]->w;
    rect.h = font[0]->h;
    SDL_BlitSurface(font[game->duration / 60], NULL, screen, &rect);
    rect.x += font[0]->w;
    SDL_BlitSurface(font[10], NULL, screen, &rect);
    rect.x += font[0]->w;
    SDL_BlitSurface(font[(game->duration % 60) / 10], NULL, screen, &rect);
    rect.x += font[0]->w;
    SDL_BlitSurface(font[game->duration % 10], NULL, screen, &rect);
    /* Draw the descriptions */
    rect.x = 420;
    rect.y = 440;
    rect.w = font3[0]->w;
    rect.h = font3[0]->h;
    for (x = 0; x < 15; x++) {
      if ((selectedline == 1) && (selectedchar == x)) {
          SDL_FillRect(screen, &rect, 0xFF0000L);
        } else {
          SDL_FillRect(screen, &rect, 0x303030L);
      }
      if ((game->level_desc_line1[x] <= 'Z') && (game->level_desc_line1[x] >= 'A')) {
        SDL_BlitSurface(font3[game->level_desc_line1[x] - 'A'], NULL, screen, &rect);
      }
      rect.x += font3[0]->w;
    }
    rect.x = 420;
    rect.y += font3[0]->h + 2;
    for (x = 0; x < 15; x++) {
      if ((selectedline == 2) && (selectedchar == x)) {
          SDL_FillRect(screen, &rect, 0xFF0000L);
        } else {
          SDL_FillRect(screen, &rect, 0x303030L);
      }
      if ((game->level_desc_line2[x] <= 'Z') && (game->level_desc_line2[x] >= 'A')) {
        SDL_BlitSurface(font3[game->level_desc_line2[x] - 'A'], NULL, screen, &rect);
      }
      rect.x += font3[0]->w;
    }
    /* Draw the playfield or solution (depends of the current mode) */
    if (viewmode == 0) { /* draw playfield */
        for (y = 0; y < 64; y++) {
          for (x = 0; x < 64; x++) {
            if ((game->field[x][y] & field_type) == field_atom) {
                tile = atom[game->field[x][y] & field_index];
              } else if ((game->field[x][y] & field_type) == field_wall) {
                tile = wall[game->field[x][y] & field_index];
              } else if ((game->field[x][y] & field_type) == field_free) {
                tile = empty;
              } else {
                tile = NULL;
            }
            if (tile != NULL) {
              rect.x = 32 + (x * tile->w);
              rect.y = 32 + (y * tile->h);
              rect.w = tile->w;
              rect.h = tile->h;
              SDL_BlitSurface(empty, NULL, screen, &rect);
              SDL_BlitSurface(tile, NULL, screen, &rect);
            }
          }
        }
      } else { /* draw solution */
        for (y = 0; y < 32; y++) {
          for (x = 0; x < 32; x++) {
            if ((game->solution[x][y] & field_type) == field_atom) {
                tile = atom[game->solution[x][y] & field_index];
              } else if ((game->solution[x][y] & field_type) == field_wall) {
                tile = wall[game->field[x][y] & field_index];
              } else if ((game->solution[x][y] & field_type) == field_free) {
                tile = empty;
              } else {
                tile = NULL;
            }
            if (tile != NULL) {
              rect.x = 32 + (x * tile->w);
              rect.y = 32 + (y * tile->h);
              rect.w = tile->w;
              rect.h = tile->h;
              SDL_BlitSurface(empty, NULL, screen, &rect);
              SDL_BlitSurface(tile, NULL, screen, &rect);
            }
          }
        }
    }
    /* draw the cursor type */
    rect.x = 600;
    rect.y = 260;
    rect.w = cursor[0]->w;
    rect.h = cursor[0]->h;
    SDL_BlitSurface(cursor[game->cursortype], NULL, screen, &rect);
    /* draw the cursor */
    rect.x = 32 + (cursorx * cursor[0]->w);
    rect.y = 32 + (cursory * cursor[0]->h);
    rect.w = cursor[0]->w;
    rect.h = cursor[0]->h;
    SDL_BlitSurface(cursor[0], NULL, screen, &rect);
    /* Refresh the screen */
    SDL_Flip(screen);
    /* Wait for next event */
    SDL_WaitEvent(&event);
    if (event.type == SDL_QUIT) {
        exitflag = 1;
      } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            exitflag = 1;
            break;
          case SDLK_LEFT:
            if (cursorx > 0) cursorx -= 1;
            break;
          case SDLK_RIGHT:
            if (cursorx < 63) cursorx += 1;
            break;
          case SDLK_UP:
            if (cursory > 0) cursory -= 1;
            break;
          case SDLK_DOWN:
            if (cursory < 63) cursory += 1;
            break;
          case SDLK_SPACE:
            if (viewmode == 0) { /* if current view is on playfield... */
                if ((game->field[cursorx][cursory] & field_type) == field_atom) {
                    game->field[cursorx][cursory] &= field_index; /* zero out the tile type */
                    game->field[cursorx][cursory] += 1;
                    if (game->field[cursorx][cursory] > 48) game->field[cursorx][cursory] = 0;
                    game->field[cursorx][cursory] |= field_atom;
                    lastitem = game->field[cursorx][cursory];
                  } else if ((game->field[cursorx][cursory] & field_type) == field_wall) {
                    game->field[cursorx][cursory] &= field_index; /* zero out the tile type */
                    game->field[cursorx][cursory] += 1;
                    if (game->field[cursorx][cursory] > 18) game->field[cursorx][cursory] = 0;
                    game->field[cursorx][cursory] |= field_wall;
                    lastitem = game->field[cursorx][cursory];
                }
              } else {  /* otherwise we are editing the solution */
                if ((game->solution[cursorx][cursory] & field_type) == field_atom) {
                    game->solution[cursorx][cursory] &= field_index; /* zero out the tile type */
                    game->solution[cursorx][cursory] += 1;
                    if (game->solution[cursorx][cursory] > 48) {
                        game->solution[cursorx][cursory] = 0;
                      } else {
                        game->solution[cursorx][cursory] |= field_atom;
                    }
                    lastitem = game->solution[cursorx][cursory];
                  } else {
                    game->solution[cursorx][cursory] = field_atom;
                    lastitem = game->solution[cursorx][cursory];
                }
            }
            break;
          case SDLK_INSERT: /* repeat the last item */
            if (viewmode == 0) { /* if current view is on playfield... */
                game->field[cursorx][cursory] = lastitem;
              } else {  /* otherwise we are editing the solution */
                game->solution[cursorx][cursory] = lastitem;
            }
            break;
          case SDLK_RETURN:
            if (viewmode == 0) { /* only if editing field */
              if ((game->field[cursorx][cursory] & field_type) == field_free) {
                  game->field[cursorx][cursory] = field_wall;
                } else if ((game->field[cursorx][cursory] & field_type) == field_wall) {
                  game->field[cursorx][cursory] = field_atom;
                } else if ((game->field[cursorx][cursory] & field_type) == field_atom) {
                  game->field[cursorx][cursory] = 0;
                } else {
                  game->field[cursorx][cursory] = field_free;
              }
              lastitem = game->field[cursorx][cursory];
            }
            break;
          case SDLK_DELETE:
            if (viewmode == 0) {
                game->field[cursorx][cursory] = 0;
              } else {
                game->solution[cursorx][cursory] = 0;
            }
            break;
          case SDLK_TAB:
            viewmode = 1 - viewmode;
            break;
          case SDLK_KP_MINUS:
            if (game->duration > 0) game->duration -= 1;
            break;
          case SDLK_KP_PLUS:
            if (game->duration < 3600) game->duration += 1;
            break;
          case SDLK_F1:
            selectedchar++;
            if (selectedchar >= 15) {
              selectedline = 3 - selectedline;
              selectedchar = 0;
            }
            break;
          case SDLK_F2:
            game->cursortype += 1;
            if (game->cursortype > 2) game->cursortype = 1;
            break;
          case SDLK_F3:
            game->bg += 1;
            if (game->bg > 2) game->bg = 0;
            break;
          case SDLK_F5:
            savelevel(game, level);
            puts("SAVED");
            break;
          default:
            if (((event.key.keysym.unicode >= 'A') && (event.key.keysym.unicode <= 'Z')) || (event.key.keysym.unicode == '.')) {
              if (selectedline == 1) {
                  if (event.key.keysym.unicode == '.') {
                      game->level_desc_line1[selectedchar] = 0;
                    } else {
                      game->level_desc_line1[selectedchar] = event.key.keysym.unicode;
                  }
                } else {
                  if (event.key.keysym.unicode == '.') {
                      game->level_desc_line2[selectedchar] = 0;
                    } else {
                      game->level_desc_line2[selectedchar] = event.key.keysym.unicode;
                  }
              }
            }
            break;
        }
    }
    if (exitflag != 0) break;
  }

  free(game);

  return(0);
}
