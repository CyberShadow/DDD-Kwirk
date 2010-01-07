int main()
{
    FILE *f = fopen("Kwirk (UA) [!].vbm", "rb");
    fseek(f, 0x100 +   309*2, SEEK_SET);  //  0:  220 + 395
    //fseek(f, 0x100 +   924*2, SEEK_SET);  //  1:  176 + 395
    //fseek(f, 0x100 +  1495*2, SEEK_SET);  //  2:  207 + 395
    //fseek(f, 0x100 +  2097*2, SEEK_SET);  //  3:  239 + 395
    //fseek(f, 0x100 +  2731*2, SEEK_SET);  //  4:  300 + 395
    //fseek(f, 0x100 +  3426*2, SEEK_SET);  //  5:  461 + 395
    //fseek(f, 0x100 +  4282*2, SEEK_SET);  //  6:  466 + 395
    //fseek(f, 0x100 +  5143*2, SEEK_SET);  //  7:  442 + 395
    //fseek(f, 0x100 +  5980*2, SEEK_SET);  //  8:  254 + 395
    //fseek(f, 0x100 +  6629*2, SEEK_SET);  //  9:  472 + 395 + 240
    //fseek(f, 0x100 +  7736*2, SEEK_SET);  // 10:  414 + 395
    //fseek(f, 0x100 +  8545*2, SEEK_SET);  // 11: 2252 + 395
    //fseek(f, 0x100 + 11192*2, SEEK_SET);  // 12: 1049 + 395
    //fseek(f, 0x100 + 12636*2, SEEK_SET);  // 13: 2481 + 396
    //fseek(f, 0x100 + 15513*2, SEEK_SET);  // 14: 1044 + 396
    //fseek(f, 0x100 + 16953*2, SEEK_SET);  // 15: 1045 + 395
    //fseek(f, 0x100 + 18393*2, SEEK_SET);  // 16: 2315 + 395
    //fseek(f, 0x100 + 21103*2, SEEK_SET);  // 17: 2639 + 396
    //fseek(f, 0x100 + 24138*2, SEEK_SET);  // 18: 1505 + 395
    //fseek(f, 0x100 + 26038*2, SEEK_SET);  // 19:  312 + 395 + 240
    //fseek(f, 0x100 + 26985*2, SEEK_SET);  // 20:  971 + 395
    //fseek(f, 0x100 + 28351*2, SEEK_SET);  // 21: 1859 + 395
    //fseek(f, 0x100 + 30605*2, SEEK_SET);  // 22: 1212 + 395
    //fseek(f, 0x100 + 32212*2, SEEK_SET);  // 23: 3484 + 395
    //fseek(f, 0x100 + 36091*2, SEEK_SET);  // 24:  461 + 396
    //fseek(f, 0x100 + 36948*2, SEEK_SET);  // 25: 3045 + 397
    //fseek(f, 0x100 + 40390*2, SEEK_SET);  // 26: 3580 + 394
    //fseek(f, 0x100 + 44364*2, SEEK_SET);  // 27: 2864 + 397
    //fseek(f, 0x100 + 47625*2, SEEK_SET);  // 28: 4061 + 394
    //fseek(f, 0x100 + 52080*2, SEEK_SET);  // 29: 4513

	int frames = 0;
    int steps = 0;
    int switches = 0;
	try
	{
		State initialState;
		initialState.load();

		State state = initialState;
        printf("None\n");
        while (state.playersLeft())
        {
            Action action;
            uint16_t input;
            fread(&input,sizeof(input),1,f);
            switch (input)
            {
            case 0x0004: action=SWITCH; switches++; break;
            case 0x0010: action=RIGHT;  break;
            case 0x0020: action=LEFT;   break;
            case 0x0040: action=UP;     break;
            case 0x0080: action=DOWN;   break;
            default: throw format("Unknown input 0x%04X", input);
            }
		    printf("%s%s\n", state.toString(), actionNames[action]);
			int res = state.perform(action);
			if (res <= 0)
				error("Bad action!");
            fseek(f, 2*(res-1), SEEK_CUR);
			frames += res;
            steps++;
        }
        /*
		for (int i=0; i<(sizeof actions / sizeof Action); i++)
		{
		    printf("%s%s\n", state.toString(), actionNames[actions[i]]);
			int res = state.perform(actions[i]);
			if (res <= 0)
				error("Bad action!");
			frames += res;
		}
        */
		printf("%s", state.toString());
		//printf("%d steps, %d frames\n", (sizeof actions / sizeof Action), frames);
	}
	catch (const char* s)
	{
		puts(s);
	}
    printf("%d+%d steps, %d frames (%1.3f seconds)\n", steps-switches, switches, frames, frames/60.);

    fclose(f);
}
