/* Implement this class. */

import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import static java.util.Comparator.*;
import static java.util.Comparator.naturalOrder;

public class MyDispatcher extends Dispatcher {
    public MyDispatcher(SchedulingAlgorithm algorithm, List<Host> hosts) {
        super(algorithm, hosts);
    }
    AtomicInteger last = new AtomicInteger(0);
    @Override
    public void addTask(Task task) {

        switch(algorithm) {
            case ROUND_ROBIN:
                hosts.get(last.get()).addTask(task);
                int r = last.getAndAdd(1);
                last.compareAndSet(hosts.size(), 0);
                break;
            case SHORTEST_QUEUE:
                    // face minimul pe cozi
                    Host hostWithMinQueue = Collections.min(hosts, new Comparator<Host>() {
                            @Override
                            public int compare(Host o1, Host o2) {
                                return o1.getQueueSize() - o2.getQueueSize();
                            }
                        });
                hostWithMinQueue.addTask(task);
                    // }
                break;
            case SIZE_INTERVAL_TASK_ASSIGNMENT:
                if (task.getType() == TaskType.SHORT) {
                    hosts.get(0).addTask(task);
                } else if (task.getType() == TaskType.MEDIUM) {
                    hosts.get(1).addTask(task);
                } else if (task.getType() == TaskType.LONG) {
                    hosts.get(2).addTask(task);
                }
                break;
            case LEAST_WORK_LEFT:
                if (task.getId() == 0) {
                    hosts.get(0).addTask(task);
                } else {
                    // face minimul pe cat mai are de lucrat
                    MyHost hostWithLWL = (MyHost) Collections.min(hosts, comparingLong(Host::getWorkLeft));
                    hostWithLWL.addTask(task);
                }
                break;
        }
    }
}
