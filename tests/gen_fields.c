#include <stdio.h>

#define O(X) putchar('<'+(X))
#define S putchar('\n')
#define B(U,L) for (int i = (U); i >= (L); i--) O(i)

int main()
{
    B(31,12);
    S; S;
    O(20);
    B(10,1);
    O(11);
    B(19,12);
    S; S;
    B(11,0);
    S; S;
    O(12);
    B(10,5);
    S;
    B(4,1);
    O(11);
    S; S;
    B(11,5);
    S;
    B(4,0);
    S; S;
    return 0;
}
