# -*- mode: ruby -*-
# vi: set ft=ruby :

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure("2") do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://vagrantcloud.com/search.
  config.vm.box = "ubuntu/bionic64"

  # Disable automatic box update checking. If you disable this, then
  # boxes will only be checked for updates when the user runs
  # `vagrant box outdated`. This is not recommended.
  # config.vm.box_check_update = false

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  # NOTE: This will enable public access to the opened port
  # config.vm.network "forwarded_port", guest: 80, host: 8080

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine and only allow access
  # via 127.0.0.1 to disable public access
  # config.vm.network "forwarded_port", guest: 80, host: 8080, host_ip: "127.0.0.1"

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
  config.vm.network "private_network", ip: "192.168.33.10"

  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
  # config.vm.network "public_network"

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.
  # config.vm.synced_folder "../data", "/vagrant_data"

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # Example for VirtualBox:
  #
  config.vm.provider "virtualbox" do |vb|
  #   # Display the VirtualBox GUI when booting the machine
     vb.gui = true
  #
  #   # Customize the amount of memory on the VM:
     vb.memory = "4096"
     vb.cpus = "2"
     vb.customize ['modifyvm', :id, '--clipboard', 'bidirectional']
   end
  #
  # View the documentation for the provider you are using for more
  # information on available options.

  # Enable provisioning with a shell script. Additional provisioners such as
  # Puppet, Chef, Ansible, Salt, and Docker are also available. Please see the
  # documentation for more information about their specific syntax and use.
  #
  # Install ns3 dependencies
  config.vm.provision "shell", inline: <<-SHELL
     apt-get update -y
     apt-get install xubuntu-desktop -y
     apt-get install gcc g++ python -y
     apt-get install gcc g++ python python-dev -y
     apt-get install python-setuptools git mercurial -y
     apt-get install qt5-default mercurial -y
     apt-get install gir1.2-goocanvas-2.0 python-gi python-gi-cairo python-pygraphviz python3-gi python3-gi-cairo python3-pygraphviz gir1.2-gtk-3.0 ipython ipython3   -y
     apt-get install openmpi-bin openmpi-common openmpi-doc libopenmpi-dev -y
     apt-get install autoconf cvs bzr unrar -y
     apt-get install gdb valgrind  -y
     apt-get install uncrustify -y
     apt-get install doxygen graphviz imagemagick -y
     apt-get install texlive texlive-extra-utils texlive-latex-extra texlive-font-utils texlive-lang-portuguese dvipng latexmk -y
     apt-get install python-sphinx dia  -y
     apt-get install gsl-bin libgsl-dev libgsl23 libgslcblas0 -y
     apt-get install tcpdump -y
     apt-get install sqlite sqlite3 libsqlite3-dev -y
     apt-get install libxml2 libxml2-dev -y
     apt-get install cmake libc6-dev libc6-dev-i386 libclang-dev llvm-dev automake pip
     pip install cxxfilt
     apt-get install libgtk2.0-0 libgtk2.0-dev -y
     apt-get install vtun lxc -y
     apt-get install libboost-signals-dev libboost-filesystem-dev -y
   SHELL

  # Build ns3
   config.vm.provision "shell", inline: <<-SHELL
     wget https://www.nsnam.org/release/ns-allinone-3.29.tar.bz2
     tar xjf ns-allinone-3.29.tar.bz2
     cd ns-allinone-3.29
     ./build.py --enable-examples --enable-tests
     cd ns-3.29
     ./test.py
     ./waf --run hello-simulator
   SHELL
end
