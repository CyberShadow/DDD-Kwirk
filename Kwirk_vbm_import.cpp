// To run this instead of a DDD search, add the following line to config.h:
// #define PROBLEM_RELATED Kwirk_vbm_import

#define MODIFY_LEVEL_11

void import_vbm()
{
    int Nitrodon_TAS_start_frame[] = {
    /*  0 */   309, // +  220 + 395
    /*  1 */   924, // +  176 + 395
    /*  2 */  1495, // +  207 + 395
    /*  3 */  2097, // +  239 + 395
    /*  4 */  2731, // +  300 + 395
    /*  5 */  3426, // +  461 + 395
    /*  6 */  4282, // +  466 + 395
    /*  7 */  5143, // +  442 + 395
    /*  8 */  5980, // +  254 + 395
    /*  9 */  6629, // +  472 + 395 + 240
    /* 10 */  7736, // +  414 + 395
    /* 11 */  8545, // + 2252 + 395
    /* 12 */ 11192, // + 1049 + 395
    /* 13 */ 12636, // + 2481 + 396
    /* 14 */ 15513, // + 1044 + 396
    /* 15 */ 16953, // + 1045 + 395
    /* 16 */ 18393, // + 2315 + 395
    /* 17 */ 21103, // + 2639 + 396
    /* 18 */ 24138, // + 1505 + 395
    /* 19 */ 26038, // +  312 + 395 + 240
    /* 20 */ 26985, // +  971 + 395
    /* 21 */ 28351, // + 1859 + 395
    /* 22 */ 30605, // + 1212 + 395
    /* 23 */ 32212, // + 3484 + 395
    /* 24 */ 36091, // +  461 + 396
    /* 25 */ 36948, // + 3045 + 397
    /* 26 */ 40390, // + 3580 + 394
    /* 27 */ 44364, // + 2864 + 397
    /* 28 */ 47625, // + 4061 + 394
    /* 29 */ 52080, // + 4513
    };

    int level_11_modification_start = 161;
    Action level_11_modification[] = {
        UP,UP,LEFT,LEFT,LEFT,LEFT,UP,UP,LEFT,LEFT,UP,UP,RIGHT,DOWN,DOWN,LEFT,DOWN,RIGHT,RIGHT,UP,RIGHT,DOWN,
        LEFT,DOWN,RIGHT,RIGHT,UP,RIGHT,DOWN,RIGHT,DOWN,DOWN,DOWN,DOWN,DOWN,DOWN,
        DOWN,LEFT,DOWN,LEFT,LEFT,LEFT,LEFT,UP
    };

    FILE *vbm = fopen("Kwirk (UA) [!].vbm", "rb");
    fseek(vbm, 0x100 + Nitrodon_TAS_start_frame[LEVEL]*2, SEEK_SET);

#if defined(MODIFY_LEVEL_11) && (LEVEL == 11)
    FILE *solution = fopen(STRINGIZE(LEVEL)"_vbm_mod.txt", "wt");
#else
    FILE *solution = fopen(STRINGIZE(LEVEL)"_vbm.txt", "wt");
#endif

	int frames = 0;
    int steps = 0;
    int switches = 0;
	try
	{
		State initialState = State::initial;

		State state = initialState;
        fputs(actionNames[NONE], solution);
        fputc('\n', solution);
        while (state.playersLeft())
        {
            Action action;
#if defined(MODIFY_LEVEL_11) && (LEVEL == 11)
            if (steps >= level_11_modification_start)
            {
                action = level_11_modification[steps - level_11_modification_start];
            }
            else
#endif
            {
                uint16_t input;
                fread(&input,sizeof(input),1,vbm);
                switch (input)
                {
                case 0x0004: action=SWITCH; switches++; break;
                case 0x0010: action=RIGHT;  break;
                case 0x0020: action=LEFT;   break;
                case 0x0040: action=UP;     break;
                case 0x0080: action=DOWN;   break;
                default: throw format("Unknown input 0x%04X", input);
                }
            }
		    fprintf(solution, "%s%s\n", state.toString(), actionNames[action]);
			int res = state.perform<true,false>(action);
			if (res <= 0)
				error("Bad action!");
            fseek(vbm, 2*(res-1), SEEK_CUR);
			frames += res;
            steps++;
        }
		fprintf(solution, "%s", state.toString());
		printf(           "%s", state.toString());
	}
	catch (const char* s)
	{
        fputs(s, solution);
        fputc('\n', solution);
		puts(s);
	}
    fprintf(solution, "Total %d+%d steps, %d frames (%1.3f seconds)\n", steps-switches, switches, frames, frames/60.);
    printf(           "Total %d+%d steps, %d frames (%1.3f seconds)\n", steps-switches, switches, frames, frames/60.);

    fclose(vbm);
    fclose(solution);
}

int run_related(int argc, const char* argv[])
{
    import_vbm();
    return 0;
}
