/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef lint
static char sccsid[] = "@(#)play_level.c	5.3 (Berkeley) 3/9/88";
#endif /* not lint */

# include	"robots.h"

/*
 * play_level:
 *	Let the player play the current level
 */
play_level()
{
	register COORD	*cp;
	register int	y, x, bonus;

	move(My_pos.y, My_pos.x);
	addch(PLAYER);
	refresh();
	for (cp = Robots; cp < &Robots[MAXROBOTS]; cp++) {
		if (cp->y < 0)
			continue;
		move(cp->y, cp->x);
		addch(ROBOT);
	}
	refresh();
# ifdef DEBUG
	standout();
	move(Min.y, Min.x);
	addch(inch());
	move(Max.y, Max.x);
	addch(inch());
	standend();
# endif DEBUG
	setjmp(End_move);
	flush_in();
	while (!Dead && Num_robots > 0) {
		move(My_pos.y, My_pos.x);
		if (!jumping())
			refresh();
		get_move();
		if (Real_time)
			alarm(0);
		if (Field[My_pos.y][My_pos.x] != 0)
			Dead = TRUE;
		if (!Dead)
			move_robots(FALSE);
		if (Was_bonus) {
			move(Y_PROMPT, X_PROMPT);
			clrtoeol();
			move(Y_PROMPT + 1, X_PROMPT);
			clrtoeol();
			Was_bonus = FALSE;
		}
	}

	/*
	 * if the player didn't die, add on the possible bonuses
	 */

	if (!Dead) {
		Was_bonus = FALSE;

		if (Level == Start_level && Start_level > 1) {
			move(Y_PROMPT, X_PROMPT);
			printw("Advance bonus: %d", S_BONUS);
			refresh();
			add_score(S_BONUS);
			Was_bonus = TRUE;
		}

		if (Wait_bonus != 0) {
			if (!Was_bonus)
				move(Y_PROMPT, X_PROMPT);
			else
				move(Y_PROMPT + 1, X_PROMPT);
			printw("Wait bonus: %d", Wait_bonus);
			refresh();
			add_score(Wait_bonus);
			Was_bonus = TRUE;
		}
	}
}
