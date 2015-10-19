Vagrant.configure(2) do |config|
  config.vm.box = 'bento/ubuntu-15.04'
  config.vm.box_check_update = true
  config.vm.provider 'virtualbox' do |c|
    c.memory = 2048
  end
  config.vm.network 'forwarded_port', guest: 10_000, host: 10_000

  Dir['vagrant/*', 'vagrant/.bash_aliases', 'vagrant/.ssh/*'].each do |source|
    destination = source.sub('vagrant', '/home/vagrant/provisioned')
    config.vm.provision 'file', source: source, destination: destination
  end
  config.vm.provision 'shell', path: 'vagrant/provision.sh', privileged: false
end
