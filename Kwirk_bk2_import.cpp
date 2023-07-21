// To run this instead of a DDD search, add the following line to config.h:
// #define PROBLEM_RELATED Kwirk_vbm_import

//#define DEBUG_PRINT
#ifdef DEBUG_PRINT
bool debug_print = false;
#endif

#define INPUT_UP     0x0001
#define INPUT_DOWN   0x0002
#define INPUT_LEFT   0x0004
#define INPUT_RIGHT  0x0008
#define INPUT_START  0x0010
#define INPUT_SELECT 0x0020
#define INPUT_B      0x0040
#define INPUT_A      0x0080
#define INPUT_POWER  0x0100

uint16_t get_input(FILE *bk2)
{
    char input_line[20];
    fgets(input_line, sizeof(input_line), bk2);
#ifdef DEBUG_PRINT
    if (debug_print)
        fputs(input_line, stdout);
#endif
    uint16_t result = 0;
    for (int i=0; i<9; i++)
        if (input_line[1 + i] != '.')
            result += 1 << i;
    return result;
}

void import_bk2()
{
    FILE *bk2 = fopen("nitrodon,zenicreverie,alyoshav2-kwirk-goingup/Input Log.txt", "rt");

    char input_line[1024];
    fgets(input_line, sizeof(input_line), bk2);
    fgets(input_line, sizeof(input_line), bk2);
    uint16_t input;
    do input = get_input(bk2); while (input != INPUT_START                    ); // Start
    do input = get_input(bk2); while (input != INPUT_A                        ); // SELECT GAME -> GOING UP
    do input = get_input(bk2); while (input != INPUT_A && input != INPUT_START); // SELECT SKILL -> EASY
    do input = get_input(bk2); while (input != INPUT_A                        ); // SELECT FLOOR -> FL# 1
    do input = get_input(bk2); while (input != INPUT_A && input != INPUT_START); // SELECT DISPLAY -> DIAGONAL VIEW (or BIRD'S-EYE VIEW)

    for (int i=0; i<LEVEL; i++)
    {
        do input = get_input(bk2); while (input != INPUT_A    ); // GOING UP? -> START
        do input = get_input(bk2); while (input != INPUT_START); // Start
        if (i % 10 == 9)
            do input = get_input(bk2); while (input != INPUT_START); // Start
    }
#ifdef DEBUG_PRINT
    debug_print = true;
#endif
    do input = get_input(bk2); while (input != INPUT_A); // GOING UP? -> START
    int delay = 169;
    {{}} if (LEVEL == 5 || LEVEL == 27)
        delay--;
    else if (LEVEL == 10 || LEVEL == 28)
        delay++;
    for (int i=0; i<delay; i++)
    {
        fgets(input_line, sizeof(input_line), bk2);
#ifdef DEBUG_PRINT
        if (debug_print)
            fputs(input_line, stdout);
#endif
    }

    FILE *solution = fopen(STRINGIZE(LEVEL)"_bk2.txt", "wt");

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
#ifdef DEBUG_PRINT
            if (debug_print)
                printf("%3d: ", steps);
#endif
            switch (input = get_input(bk2))
            {
            case 0x0020: action=SWITCH; switches++; break;
            case 0x0008: action=RIGHT;  break;
            case 0x0004: action=LEFT;   break;
            case 0x0001: action=UP;     break;
            case 0x0002: action=DOWN;   break;
            case 0x0000: action=NONE;   break;
            default: throw format("Unknown input 0x%04X", input);
            }
            fprintf(solution, "%s%s\n", state.toString(), actionNames[action]);
            int res = state.perform<true,false>(action);
            if (res <= 0)
                error("Bad action!");
            int res_bk2 = res;
            if (LEVEL == 18 && action == SWITCH)
            {
                {{}} if (switches == 3)
                    res_bk2 += 2;
                else if (switches == 4)
                    res_bk2 -= 2;
            }
            else
            if (LEVEL == 27 && steps == 174)
            {
                res_bk2 -= 2;
            }
            for (int i=1; i<res_bk2; i++)
            {
                fgets(input_line, sizeof(input_line), bk2);
#ifdef DEBUG_PRINT
                if (debug_print)
                {
                    fputs("     ", stdout); fputs(input_line, stdout);
                }
#endif
            }
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

    fclose(bk2);
    fclose(solution);
}

int run_related(int argc, const char* argv[])
{
    import_bk2();
    return 0;
}
