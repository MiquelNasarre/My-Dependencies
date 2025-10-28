# My Dependencies

This repository contains three different libraries, used across multiple projects to do common tasks. 
These are an **Image** library, for simple image and color handling, a **Timer** class, to accurately 
time projects with computational overhead, and a **Thread** class, for threading functions 
inside a project and proper control of threads.

Usually when I write code I try to avoid using the standard library for C++, since it provides 
a lot of functions but I do not know or can control how they are made, and they might not fit the purpose 
of my particular project. I also try to keep my files as clean from dependencies as possible, to avoid 
unnecessary externals. That is why I constantly keep coming back to these libraries, no header 
dependencies and fully designed for my projects.

## Requirements

- **Image/Timer**: Both libraries are cross-platform, and should run on any device with a new enough
  architecture, you just need to add the source and header files in your project.

- **Thread**: This class is currently only for Windows OS, since matches current setup.

- [Visual Studio](https://visualstudio.com): Only needed if you want to build the current solution
  containing the three libraries.
  
## Image

As noted above this library is built for handling images inside C++, it has functionalities for 
loading images from files and saving them, and the **Image** object itself stores the pixels as a 
**Color** array, easily modifiable through the accessors.

This class was created for my 3D renderer, so the **Image** and **Color** objects are very useful 
as textures for renderer apps, but can also be used for simple image modification.

Let's give a simple example to understand the functionalilty, we took a screenshot with a background 
and we want to make this background transparent. First we can use external tools to turn the picture into 
a decompressed bitmap format. My favourite tool for image modifications is [imageMagick](https://imagemagick.org/), 
simply input into a console prompt:

```cmd
> magick initial_image.png -compress none image.bmp
```

Once you have the correct format you can edit the picture as much as you want, for this simple example 
we can run the following loop:

```cpp
#include "Image.h"

// Initialize image using file name
Image image("image.bmp");

// Loop through the image pixels
for (unsigned row = 0; row < image.height(); row++)
	for (unsigned col = 0; col < image.width(); col++)
	{
		Color original = image(row, col);

        // Check for green pixels and turn them transparent
		if (original == Color::Green)
			image(row, col) = Color::Transparent;
        // Otherwise shuffle the channels
		else
		{
			image(row, col).R = original.G;
			image(row, col).G = original.B;
			image(row, col).B = original.R;
		}
	}

// Save resulting picture to new file
image.save("transparented_image.bmp");
```

Following the previous steps I turned a screenshot of my Fourier Series program from green, 
to transparent background and shuffled the channels:

---
<img width="902" height="516" alt="image" src="https://github.com/user-attachments/assets/909a2067-3bdb-4ddc-ab58-bb93237f95c3" />

---
This is a simple example, but with direct pixel access, your imagination is the limit!

## Timer

Very straight forward cross-platform timer class, also created for my 3D renderer in order to measure
FPS inside the program. It has basic functions easily learned by reading the header.

Let's give an example where we want to run a very time-consuming function multiple times inside a loop 
to give a basic overview of its functionality:

```cpp
#include "Timer.h"

Timer timer;

// Set the maximum number of stored marks to cover the entire scope.
timer.setMax(total_iterations);
for(unsigned i = 0; i < total_iterations; i++)
{
  // Run the time-consuming function
  time_consuming_function();

  // Check time elapsed while the function was running
  float time = timer.mark();
  printf("iteration %u finished after %.4fs\n", i, time);
}

// Check the average across iterations
float average = timer.average();
printf("Average time per computation was %.4fs\n", average);

// Reset the timer ready for next task
timer.reset();
```

The class also comes with a set of very handy static functions like precision sleepers, so that 
you can easily put the current thread to sleep, and a system time getter, ideal to use as a seed 
for generating random numbers.

```cpp
unsigned long milliseconds = 235;
Timer::sleep_for(milliseconds);

unsigned long long microseconds = 14200;
Timer::sleep_for_us(microseconds);

uint64_t seed = Timer::get_system_time_ns();
set_rng_seed(seed);
```

It is an extremely helpful class and I basically use it on every single project.

## Thread

Last but not least we have the thread class, this class is more recent and came from my desire to 
multithread my Connect4 engine while avoiding the **std::thread** object, and it turns out with the 
built in Windows 32 API you can create very complex interactions between threads with some very simple 
functions.

To showcase the functionality we will create a simple worker loop using this class, let's start by 
creating a worker.

```cpp
#include "Thread.h"

struct Worker
{
    unsigned idx = 0;
    // Initialize an empty thread object inside each worker
    Thread thread;

    // Function to send the worker to work
    bool go_to_work(DATA* data, unsigned char id_call)
    {
        if (thread.start(&threaded_worker_function, data, id_call))
        {
            thread.set_name(L"Worker %u", idx);
            thread.set_affinity(CPU(idx));
            thread.set_priority(Thread::PRIORITY_HIGHEST);
            return true;
        }
        return false;
    }

    // Function to see if the worker has finished
    bool has_finished()
    {
        return thread.has_finished();
    }
};
```

This is the hard part done, now we have a simple worker than when called to work threads a function 
for us, and we can check if it has finished his work by simply calling **Thread::has_finished()**. 
Also this example shows a lot of different quality of life functions, like **Thread::set_name()**, 
**Thread::set_affinity()** or **Thread::set_priority()**.

The best of them all are the **Thread::waitForWakeUp()/Thread::wakeUpThreads()** couple, that allow 
threads to go to sleep until another thread calls them, using the same ID, and that's precisely what 
the worker thread function will do at the end of each run.

Having these concepts in mind now it is trivial to build a main loop using these workers, we can do 
the following:

```cpp
// Create workers
Worker workers[N_WORKERS];

// Initialize the data
DATA* data = initialize_data();

// Loop until finished task
while (data->end_task != true)
{
    // If workers finished send them to work
    for(Worker& w: workers)
        if(w.has_finished())
            w.go_to_work(data, MAIN_LOOP_ID);

    // Wait until someone wakes you up
    Thread::waitForWakeUp(MAIN_LOOP_ID);
}
```

As you can see using threads with this class is absolutely trivial and you can generate very complex 
interactions. There are other functions in this class typical of other thread classes like detaching, 
suspending/resuming/terminating, self-thread management... To check all the functionalities you can take 
a look at the header.

## Releases and Downloads

As mentioned before if you clone the repository you will get a Visual Studio solution with the three 
libraries and no other project. Otherwise you can just copy the source and include files and add them 
and link them to your project and they will already be running.

This repository is mostly for my own storage of this valuable resources so in the releases you can 
also find for download the templates I use in my project when I want to start a project that already 
includes the **Timer** and/or the **Thread** class, which are the ones I use the most.
