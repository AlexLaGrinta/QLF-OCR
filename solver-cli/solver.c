#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>


char** get_matrix_of_file(const char* path, size_t* num_lines, size_t* num_columns)
{
    FILE *fptr;

    fptr = fopen(path, "r");

    if (fptr == NULL)
    {
        errx(EXIT_FAILURE, "file not found.");
    }

    char line[100];

    size_t number_of_lines = 1;
    size_t number_of_columns = 0;

    fgets(line, 100, fptr);

    while (line[number_of_columns] != '\0' && line[number_of_columns] != '\n' && line[number_of_columns] != ' ')
    {
        number_of_columns++;
    }

    while(fgets(line, 100, fptr)) {
        number_of_lines++;
    }

    rewind(fptr); 

    char** grid = malloc(sizeof(char*)*number_of_lines);

    for (size_t i = 0; i < number_of_lines; i++)
    {
        grid[i] = malloc(number_of_columns+1);
    }



    size_t ind = 0;
    while(fgets(line, 100, fptr)) {
        memcpy(grid[ind++], line,number_of_columns);
        grid[ind-1][number_of_columns] = '\0';
    }

    fclose(fptr); 

    *num_lines = number_of_lines;
    *num_columns = number_of_columns;

    return grid;
}

void free_grid(char** grid, size_t lines)
{
    for (size_t i = 0; i < lines; i++)
    {
        free(grid[i]);
    }
    free(grid);
}

int search_word(char** grid, const char* word, size_t size_word, 
    size_t x, size_t y, size_t lines, size_t columns, size_t* end_x, size_t* end_y)
{
    //right
    if (x + size_word <= columns) 
    {
        int broke = 0;
        for (size_t offset = 0; offset < size_word; offset++)
        {
            if (grid[y][x+offset] != word[offset]) 
            {
                broke = 1;
                break;
            }
        } 
        if (!broke) 
        {
            *end_x = x+size_word-1;
            *end_y = y;
            return 1;
        }
    }

    // left
    if (x >= size_word - 1) 
    {
        int broke = 0;
        for (size_t offset = 0; offset < size_word; offset++)
        {
            if (grid[y][x-offset] != word[offset]) 
            {
                broke = 1;
                break;
            }
        } 
        if (!broke) 
        {
            *end_x = x-size_word+1;
            *end_y = y;
            return 1;
        }
    }

    
    // low
    if (y + size_word <= lines) 
    {
        int broke = 0;
        for (size_t offset = 0; offset < size_word; offset++)
        {
            if (grid[y+offset][x] != word[offset]) 
            {
                
                broke = 1;
                break;
            }
        } 
        if (!broke) 
        {
            *end_x = x;
            *end_y = y+size_word-1;
            return 1;
        }
    }

    //high
    if (y >= size_word - 1) 
    {
        int broke = 0;
        for (size_t offset = 0; offset < size_word; offset++)
        {
            if (grid[y-offset][x] != word[offset]) 
            {
                broke = 1;
                break;
            }
        } 
        if (!broke) 
        {
            *end_x = x;
            *end_y = y-size_word+1;
            return 1;
        }
    }

    // bottom right corner
    if (x + size_word <= columns && y + size_word <= lines) 
    {
        int broke = 0;
        for (size_t offset = 0; offset < size_word; offset++)
        {
            if (grid[y+offset][x+offset] != word[offset]) 
            {
                broke = 1;
                break;
            }
        } 
        if (!broke) 
        {
            *end_x = x+size_word-1;
            *end_y = y+size_word-1;
            return 1;
        }
    }

    //top right corner
    if (x + size_word <= columns && y >= size_word - 1) 
    {
        int broke = 0;
        for (size_t offset = 0; offset < size_word; offset++)
        {
            if (grid[y-offset][x+offset] != word[offset]) 
            {
                broke = 1;
                break;
            }
        } 
        if (!broke) 
        {
            *end_x = x+size_word-1;
            *end_y = y-size_word+1;
            return 1;
        }
    }

    //top left corner
    if (x >= size_word - 1 && y >= size_word - 1) 
    {
        int broke = 0;
        for (size_t offset = 0; offset < size_word; offset++)
        {
            if (grid[y-offset][x-offset] != word[offset]) 
            {
                broke = 1;
                break;
            }
        } 
        if (!broke) 
        {
            *end_x = x-size_word+1;
            *end_y = y-size_word+1;
            return 1;
        }
    }

    //bottom left corner
    if (x >= size_word - 1 && y + size_word  <= lines) 
    {
        int broke = 0;
        for (size_t offset = 0; offset < size_word; offset++)
        {
            if (grid[y+offset][x-offset] != word[offset]) 
            {
                broke = 1;
                break;
            }
        } 
        if (!broke) 
        {
            *end_x = x-size_word+1;
            *end_y = y+size_word-1;
            return 1;
        }
    }
    
    return 0;

}


int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        errx(EXIT_FAILURE, "missing");
    }

    const char* path = argv[1];
    const char* word = argv[2];

    size_t number_of_lines;
    size_t number_of_columns; 

    char** grid = get_matrix_of_file(path, &number_of_lines, &number_of_columns);

    size_t size_word = strlen(word);

    int res = 0;

    for (size_t y = 0; y < number_of_lines; y++)
    {
        char* line = grid[y];
        for (size_t x = 0; x < number_of_columns; x++)
        {
            if (line[x] == word[0])
            {
                size_t end_x;
                size_t end_y;
                res = search_word(grid, word,size_word,x,y,number_of_lines, 
                    number_of_columns, &end_x, &end_y);
                if (res)
                {
                    printf("(%li,%li)(%li,%li)\n", x,y,end_x,end_y);
                    break;
                }
            }
        }
        if (res) break;
    }

    if (!res) printf("Not Found\n");


    free_grid(grid, number_of_lines);

    return EXIT_SUCCESS;
}