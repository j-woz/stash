
#include <assert.h>
#include <stdio.h>

#include <buffer.h>

int
main()
{
  buffer b;
  buffer_init(&b, 4);

  bool c;
  c = buffer_appendv(&b, "hello\n");
  assert(c);
  for (int i = 0; i < 10; i++)
  {
    c = buffer_appendv(&b, "bye\n");
    assert(c);
  }

  printf("%s\n", b.data);

  printf("finalize\n");
  buffer_finalize(&b);
  printf("finalized\n");
  return 0;
}
