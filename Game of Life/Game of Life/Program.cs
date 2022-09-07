//30116 정의재
using System;
using System.Threading;

namespace Game_of_Life
{

	class Program
	{
		public int[,] cell = new int[40, 40];
		public bool[,] live = new bool[40, 40];

		public void OutCell(int x, int y)
		{
			if (x < 40 && y < 40 && x > 0 && y > 0)
			{
				cell[x + 1, y] += 1;
				cell[x + 1, y + 1] += 1;
				cell[x + 1, y - 1] += 1;

				cell[x - 1, y] += 1;
				cell[x - 1, y+1] += 1;
				cell[x - 1, y-1] += 1;

				cell[x, y + 1] += 1;
				cell[x, y - 1] += 1;


			}
		}
		public void CellReset()
		{
			for (int y = 0; y < 40; y++)
			{
				for (int x = 0; x < 40; x++)
				{
					cell[x, y] = 0;
				}
			}
		}

		public void Draw()
		{
			for (int y = 0; y < 40; y++)
			{
				for (int x = 0; x < 40; x++)
				{
					if (live[x, y])
					{
						Console.SetCursorPosition(x, y);
						Console.Write("0");

					}
				}
			}
		}

		static void Main(string[] args)
		{
			Program program = new Program();

			program.live[10, 11] = true;
			program.live[11, 10] = true;
			program.live[12, 11] = true;
			program.live[11, 11] = true;
			program.live[11, 12] = true;


			while (true)
			{
				program.Draw();
				for (int y = 0; y < 40; y++)
				{
					for (int x = 0; x < 40; x++)
					{
						if (program.live[x, y])
						{
							program.OutCell(x, y);
						}
					}
				}

				for (int y = 0; y < 40; y++)
				{
					for (int x = 0; x < 40; x++)
					{
						bool live = false;
						if (program.live[x, y])
						{
							if (program.cell[x, y] == 3 || program.cell[x, y] == 2)
							{
								live = true;
							}
						}
						else if (!program.live[x, y])
						{
							if (program.cell[x, y] == 3)
							{
								live = true;
							}
						}
						program.live[x, y] = live;
					}
				}

				program.CellReset();
				Thread.Sleep(1000);
				Console.Clear();
			}



		}
		
	}
}

//나 왜 c#으로 했지..?