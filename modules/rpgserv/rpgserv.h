/*
 * rpgserv - rpg finding service
 * Copyright (c) 2011 Atheme Development Group
 */

#ifndef __RPGSERV_H
#define __RPGSERV_H

typedef enum {
	RPG_RATING_G,
	RPG_RATING_PG,
	RPG_RATING_PG13,
	RPG_RATING_R,
	RPG_RATING_ADULT,
	RPG_RATING_COUNT,
} rpg_rating_t;

typedef enum {
	RPG_RULESET_ADND,
	RPG_RULESET_DND3,
	RPG_RULESET_DND35,
	RPG_RULESET_DND4,
	RPG_RULESET_FREEFORM,
	RPG_RULESET_HOMEBREW,
	RPG_RULESET_NWOD,
	RPG_RULESET_OWOD,
	RPG_RULESET_OTHER,
	RPG_RULESET_COUNT,
} rpg_ruleset_t;

typedef enum {
	RPG_PERIOD_ANTIQUITY,
	RPG_PERIOD_EARLYMODERN,
	RPG_PERIOD_FUTURE,
	RPG_PERIOD_MIDDLEAGES,
	RPG_PERIOD_MODERN,
	RPG_PERIOD_PREHISTORIC,
	RPG_PERIOD_COUNT,
} rpg_period_t;

typedef enum {
	RPG_GENRE_APOCALYPSE,
	RPG_GENRE_ANIME,
	RPG_GENRE_ANTROPOMORPH,
	RPG_GENRE_CYBERPUNK,
	RPG_GENRE_FANTASY,
	RPG_GENRE_HORROR,
	RPG_GENRE_MULTIGENRE,
	RPG_GENRE_REALISTIC,
	RPG_GENRE_SCIFI,
	RPG_GENRE_STEAMPUNK,
	RPG_GENRE_OTHER,
	RPG_GENRE_COUNT
} rpg_genre_t;

typedef struct {
	mychan_t *owner;

	rpg_rating_t rating;
	rpg_ruleset_t ruleset;
	rpg_period_t period;
	rpg_genre_t genre;

	char *summary;
	char *setting;
	char *storyline;
	char *system;
	char *ooc_channel;
} rpg_channel_t;

#endif
