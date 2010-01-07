// To run this instead of a DDD search, add the following line to config.h:
// #define PROBLEM_RELATED Kwirk_vgm_import

void import_vgm()
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

    FILE *vgm = fopen("Kwirk (UA) [!].vbm", "rb");
    fseek(vgm, 0x100 + Nitrodon_TAS_start_frame[LEVEL]*2, SEEK_SET);

    FILE *solution = fopen(BOOST_PP_STRINGIZE(LEVEL)"_vgm.txt", "wt");

	int frames = 0;
    int steps = 0;
    int switches = 0;
	try
	{
		State initialState;
		initialState.load();

		State state = initialState;
        fputs(actionNames[NONE], solution);
        fputc('\n', solution);
        while (state.playersLeft())
        {
            Action action;
            uint16_t input;
            fread(&input,sizeof(input),1,vgm);
            switch (input)
            {
            case 0x0004: action=SWITCH; switches++; break;
            case 0x0010: action=RIGHT;  break;
            case 0x0020: action=LEFT;   break;
            case 0x0040: action=UP;     break;
            case 0x0080: action=DOWN;   break;
            default: throw format("Unknown input 0x%04X", input);
            }
		    fprintf(solution, "%s%s\n", state.toString(), actionNames[action]);
			int res = state.perform(action);
			if (res <= 0)
				error("Bad action!");
            fseek(vgm, 2*(res-1), SEEK_CUR);
			frames += res;
            steps++;
        }
		fprintf(solution, "%s", state.toString());
		printf(           "%s", state.toString());
	}
	catch (const char* s)
	{
		puts(s);
	}
    fprintf(solution, "Total %d+%d steps, %d frames (%1.3f seconds)\n", steps-switches, switches, frames, frames/60.);
    printf(           "Total %d+%d steps, %d frames (%1.3f seconds)\n", steps-switches, switches, frames, frames/60.);

    fclose(vgm);
    fclose(solution);
}

int run_related(int argc, const char* argv[])
{
    import_vgm();
    return 0;
}
