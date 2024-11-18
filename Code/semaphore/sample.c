int finished = 0;
main()
{
  spawn( task1 );
  spawn( task2 );
  spawn( task3 );
  while( finished !=3)
  {
    sleep(100);
  }
  printf("done\n");
}

void task1 (void)
{
  compute();
  finished++;
}

void task2 (void)
{
  solve();
  finished++;
}

void task3 (void)
{
  find();
  finished++;
}