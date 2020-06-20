Vagrant.configure("2") do |config|
    config.vm.box = "ubuntu/bionic64"
    config.vm.provision :shell, path: "Vagrantfile_bootstrap.sh"

    config.vm.synced_folder ".", "/home/vagrant/project"
    config.vm.synced_folder "C:/nRF5/nRF5_SDK_16.0.0_98a08e2", "/home/vagrant/sdk"
end