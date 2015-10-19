# sudo apt-get update
# sudo apt-get install -y openjdk-8-jre
#
# wget -q http://www.scala-lang.org/files/archive/scala-2.10.4.deb
# sudo dpkg -i scala-2.10.4.deb
# sudo apt-get -f -y install
#
# HADOOP_VERSION=hadoop-2.7.1
# [ -e "$HADOOP_VERSION".tar.gz ] || \
#   wget -q http://apache.arvixe.com/hadoop/common/"$HADOOP_VERSION"/"$HADOOP_VERSION".tar.gz
# rm -rf hadoop "$HADOOP_VERSION"
# tar xzf "$HADOOP_VERSION".tar.gz
# ln -s "$HADOOP_VERSION" hadoop
# rm -f "$HADOOP_VERSION".tar.gz
#
# SPARK_VERSION=spark-1.5.0-bin-hadoop2.6
# [ -e "$SPARK_VERSION".tgz ] || \
#   wget -q http://apache.arvixe.com/spark/spark-1.5.0/"$SPARK_VERSION".tgz
# rm -rf spark "$SPARK_VERSION"
# tar xzf "$SPARK_VERSION".tgz
# ln -s "$SPARK_VERSION" spark
# rm -f "$SPARK_VERSION".tgz

# provision() {
#   mkdir -p $(dirname /home/vagrant/"$1")
#   rm -f "$1" && ln -s /home/vagrant/provisioned/"$1" /home/vagrant/"$1"
# }
#
# provision .bash_aliases
# provision .ssh/config
# provision hadoop/etc/hadoop/core-site.xml
# provision hadoop/etc/hadoop/hadoop-env.sh
# provision hadoop/etc/hadoop/hdfs-site.xml
# provision spark/conf/spark-defaults.conf
# provision spark/conf/spark-env.sh

. .bash_aliases
mkdir -p data/{name,data}node

if [ ! -e /home/vagrant/.ssh/id_rsa ]; then
  ssh-keygen -t rsa -P '' -f /home/vagrant/.ssh/id_rsa
  cat /home/vagrant/.ssh/id_rsa.pub >> /home/vagrant/.ssh/authorized_keys
fi

hadoop/sbin/start-dfs.sh
[ -e /home/vagrant/data/namenode/current/VERSION ] || \
  hadoop/bin/hdfs namenode -format
spark/sbin/start-all.sh
spark/sbin/start-thriftserver.sh
