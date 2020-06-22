# BITalino Server
System for acquisition of the biosignals from BITalino board.

## 1 - Architecture:
This topic introduce the project developed, describing each part of the topology and how the data flows. This project has as objective to implement a system for acquisition of the biosignals from the BITalino board via bluetooth, these signals are received and processed, after the processing, the data is stored in the database and can be viewed in a web dashboard.

#### 1.1 - Topology:

![Topology](https://github.com/wrfrohlich/BITalino-Raspberry/blob/master/figures/topology.png)

#### 1.2 - Ports:
In this project all ports are defined and changed into other range to avoid conflict and security.

* 18000 -> Web / Dashboard (Grafana) - Access the Dashboard

* 18001 -> Database (PostgreSQL) - Access the Database

* 18002 -> Processing (Python) - Socket between Acquisition application and Processing application

## 2 - How to use this project:
In this section is presented how to use this project. In the first part is introduced how to install all dependencies and prepare the enviroment to be ready to apply the project. In the next step is presented how to use and modify each service/application.

#### 2.1 - Preparing the enviroment:
First of all it is necessary install and configure the libraries for the environment are ready to applications.

2.1.1 - Bluetooth:

* Install the bluetooth library: `sudo apt-get install pi-bluetooth`

* Start the configuration: `sudo bluetoothctl`

* Scan to find bluetooth address: `scan on`

* Connect with the device: `pair xx:xx:xx:xx:xx:xx`

2.1.2 - Docker:

!!! Raspberry Pi uses the ARM architecture, so will not be compatible with all containers. Images need to be built from an ARM base !!!

* Install the docker: `curl -sSL https://get.docker.com | sh`

* Add the user: `sudo usermod -aG docker pi`

* Enable the docker to start with the system: `sudo systemctl enable docker`

* Restart the system: `sudo reboot -h now`

2.1.3 - Docker Compose:

* Install the dependencies: `sudo apt-get install -y libffi-dev libssl-dev`

* Install Python 3: `sudo apt-get install -y python3 python3-pip`

* Install Docker Compose: `sudo pip3 install docker-compose`

2.1.4 - Python Libraries:

* Install Numpy: `sudo apt-get install python3-numpy`

* Install Pandas: `sudo apt-get install python3-pandas`

* Install Biosppy: `sudo apt-get install python3-biosppy`

#### 2.2 - Using the Docker Compose:

After to prepare the enviroment to use the docker compose it is necessary to run the containers to start the database and the dashboard. The file that defines the parameters for each service is the 'docker-compose.yml', in this file is defined the network amoung services that will be the `'my-network'` with brigde caracteristic. In the next topic is defined the service, for each service is assigned an image that will be based, for the web service will be used the image `grafana/grafana:7.0.0` and the service database will use the image `postgres:9.6`.

![Docker](https://github.com/wrfrohlich/BITalino-Raspberry/blob/master/figures/docker.png)

In each service, also is declareted different ports with external visibility. To run the docker compose the path must be changed in the bash with to the folder that are the files of this project, in sequence using `cd docker` and `docker-compose up -d` to run the containers.

#### 2.3 - Using the Database (__PostgreSQL__):

Inside the folder 'database' there are two files, the first file is used to inicialize the database `'bitalino'` and the three tables, the first table is `'raw_data'` with categories `'name'`, `'ecg'`, `'eda'` and `'time'` without processing. The second table is `'processed'` with the same categories but all processed and the third table is the `'ecg'` with the ECG signal in the time `'name'`, `'heart_rate'` and `'heart_rate_ts'`. The second file is used to check if the database and table was created properly. This database created use the exposed port `18001`, user `postgres` and the password is not defined.

#### 2.4 - Using the Dashboard (__Grafana__):

Inside the folder 'web' there are other two folders. The folder 'datasources' there is the file (datasource.yaml) with the configuration of the database (PostgreSQL) that will be used to get the data to monitoring. The folder 'dashboards' there are four files, the first file (dashboard.yaml) is used to configure the dashboard properly and the others files are used to have the dashboard entirely configured, one for each database. To access the dashboard use the Raspberry's IP and port `18000` (e.g. 127.0.0.1:18000).

After initialized the Grafana's container, it is necessary to set the dashboard to visualize the database. By default the user and password are admin / admin, in the sequence you can define another password, all steps to set the predefined dashboard in the Grafana are list below in the figure.

#### 2.5 - Using the Processing (__Python__):

The Python code is responsable to receive all data from the C++ code via socket. In addition to receiving the data, this service also process and stores in the database, it is possible to define some alarms to send to cloud, in this case, the cloud plataform is the Internet of Thing Plataform from IBM, with the MQTT protocol.

In the first part of the code are defined the libraries, in the second part are difined the global variables of the code, the function `socket.socket(socket.AF_INET, socket.SOCK_DGRAM)` define the protocol and `bind('',18002)` is used to define the IP address and port of the socket server, after is defined the `DSN = 'dbname=bitalino user=postgres host=localhost port=18001'` to define the address of database and for last, the variables of the IBM Watson server. In the third part of the code is defined the functions, in this part there are one function to save a CSV file with all raw data in the main folder of the application, the second and thirdy functions are responsable to generate the processed data, in sequence, more three functions, one for each register in the database and finally, the function responsable to make send the event to the cloud, in this case is set to send in the final of the processing.

Finally, in the fourth part is the main function, a loop that opens the socket and receives the information from the acquisition application. Each data is received from the C++ code with a different caracter for identify the kind of data and is converted into the correct format. When the Python code receive a determinated signal the code call the respective function to do the action.

![IBM](https://github.com/wrfrohlich/BITalino-Raspberry/blob/master/figures/ibm.png)

#### 2.5 - Using the Acquisition (__C++__):

The first step to use the application it is define the bluetooth address in the acquisition.cpp file in the variable `BITalino dev("xx:xx:xx:xx:xx:xx")`. In the acquisition.cpp file there are others options to configure the system, for example the channels that will be monitored (channel 1 = ECG; channel 2 = EDA) or the port of the sockets, all these options are described in the code.

The next step must change the path in the bash to the path with all files, now it is possible generate the executable file using the command `make`. After this, the application is able to use with the command `./acquisition`. Turn on the BITalino and fill name and last name to start the acquisition. To finish the acquisition it is necessary tap the 'enter' in the keyboard to stop the acquisition and disconnect properly the BITalino.

## 3 - Useful Commands:

#### 3.1 - Basic Commands:
* List Images: `docker image ls`

* List Containers: `docker container ls -a`

* List Volumes: `docker volume ls`

* List Networks: `docker network ls`

* Remove Unnecessary Volumes: `docker volume prune`

* Remove Unnecessary Networks: `docker network prune`

* Remove Images: `docker image rm xxxxxx`

* Remove Containers: `docker image rm xxxxxx`

* Remove Volumes: `docker image rm xxxxxx`

* Remove Networks: `docker image rm xxxxxx`

#### 3.2 - Advanced Commands:
* Up Docker-Compose with Logs: `docker-compose up`

* Up Docker-Compose without Logs: `docker-compose up -d`

* Check Docker-Compose: `docker-compose ps`

* Down Docker-Compose: `docker-compose down`

* Logs of Docker-Compose: `docker-compose logs -t -f`

* Logs of Database Application: `docker-compose logs -t -f database`

* Logs of Dashboard Application: `docker-compose logs -t -f web`

* Check the Creation of Database: `docker-compose exec database psql -U postgres -f ./database/check.sql`

* Check the Database: `docker-compose exec database psql -U postgres -d bitalino -c 'select * from patients'`
