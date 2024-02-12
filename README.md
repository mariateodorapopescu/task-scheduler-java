# Task Scheduler in Java

the implementation of the project was realized in the MyDispatcher and MyHost classes.

## 1. MyDispatcher:

### addTask:

- I took each policy separately and tried to pass on the task to that host that fits according to the rule.
- Thus, I thought: to make a switch (I didn't want to have too many ifs, it seemed redundant) and I took the types.
- Thus, at round robin, I incremented a number that will represent the index of the host to which it will be sent, and I repeated it to 0 when the number of hosts was reached, so that it would not exceed the number of hosts and somehow cycle. I used the atomic variable so that the race condition does not occur.
- At the shortest queue, I put the queues on a collection to get the minimum, that is, the one with the shortest length, according to the comparator.
- At SITA it was quite simple, I took the function of the enum where to go, this time with if.
- At Least work left, I proceeded similarly to the shortest queue.
- Only the addTask method was to be implemented here.

## 2. MyHost:

- Instead, in the myHost class I implemented several methods, such as run, addTask, getQueueSize, getWorkLeft and shutdown, as well as the host queue.
- Basically, it is based on synchronization, using a reentrant lock to signal the critical sections and the addition or removal of the task from the queue. When a task is removed from the queue, it is run. The current, running task is not the one at the top of the queue (this only happens at the beginning).

### Queue:
Thus, we used a PriorityBlockingQueue which ensures that the tasks are ordered according to priority and, in case of equality, according to their start time.

### run:
-In the run method I simulated the running of a task, doing a busywaiting with sleep, depending on the duration. I know that in real life it doesn't happen like that, i.e. it doesn't run all the way, it runs a bit and stops, but at least it can be applied for certain conditions, like if it's not preemptible.

### addTask:
- Instead, the logic of preempting (I have an attempt to do this here) with the re-ordering according to preemption and priorities I realized in addTask.

### Shutdown:
- Shutdown is based on changing the ok variable, which indicates whether the host is running or not.

### getQueueSize:
- I turned the length of the tail and that's it.

### getWorkLeft:
- I made the sum of the left times of all the tasks in the host queue
