import java.util.*;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.PriorityBlockingQueue;
import java.util.concurrent.locks.ReentrantLock;

public class MyHost extends Host {
//    private BlockingQueue<Task> host_queue = new ArrayBlockingQueue<>(100000);
private final PriorityBlockingQueue<Task> host_queue = new PriorityBlockingQueue<>(10000, (o1, o2) -> {
    if (o1.getPriority() == o2.getPriority()) {
        return o1.getStart() - o2.getStart();
    }
    return o2.getPriority() - o1.getPriority();
});
    private int ok = 1;
    private ReentrantLock lock = new ReentrantLock();
    @Override
    public void run() {
        while (true) {
            Task running_task = null;
            synchronized (lock) {
                if (!host_queue.isEmpty()) {
                    running_task = host_queue.poll();
                }
            }
            if (running_task != null) {
                try {
                    Thread.sleep(running_task.getDuration());
                    running_task.finish();
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }
            if (ok == 0) {
                break;
            }
        }
    }
    @Override
    public void addTask(Task task) {
        synchronized (lock) {
            Task running_task = host_queue.peek();
            if (running_task != null && running_task.isPreemptible() &&
                    task.getPriority() > running_task.getPriority()) {
                Queue<Task> aux = new LinkedList<>();
                Task other_task = host_queue.poll();
                other_task.finish();
                while (!host_queue.isEmpty()) {
                    Task current_element = host_queue.poll();
                    aux.add(current_element);
                }
                host_queue.add(task);
                lock.notify();
                host_queue.add(other_task);
               while (!aux.isEmpty()) {
                   Task current_element = aux.poll();
                   host_queue.add(current_element);
               }
            } else if (running_task != null && running_task.isPreemptible() == false &&
                    task.getPriority() > running_task.getPriority()) {
                Queue<Task> aux = new LinkedList<>();
                Task other_task = host_queue.poll();
                while (!host_queue.isEmpty()) {
                    Task current_element = host_queue.poll();
                    aux.add(current_element);
                }
                host_queue.add(task);
                lock.notify();
                host_queue.add(other_task);
                while (!aux.isEmpty()) {
                    Task current_element = aux.poll();
                    host_queue.add(current_element);
                }

            } else {
                host_queue.add(task);
                lock.notify();
            }
        }
    }
    @Override
    public int getQueueSize() {
        synchronized (lock) {
            return host_queue.size();
        }
    }
    @Override
    public long getWorkLeft() {
        synchronized (lock) {
            long s = 0;
            for (Task t : host_queue) {
                s = s + t.getLeft();
            }
            return s;
        }
    }
    @Override
    public void shutdown() {
        synchronized (lock) {
            ok = 0;
        }
    }
}