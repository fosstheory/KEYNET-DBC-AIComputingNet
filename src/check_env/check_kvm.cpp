#include "check_kvm.h"
#include "tinyxml2.h"
#include <sstream>
#include <uuid/uuid.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include "util/SystemResourceManager.h"
#include <sys/sysinfo.h>
#include "virImpl.h"

namespace check_kvm {
    static const char* qemu_url = "qemu+tcp://localhost:16509/system";

    static std::vector<std::string> SplitStr(const std::string &s, const char &c) {
        std::string buff;
        std::vector<std::string> v;
        char tmp;
        for (int i = 0; i < s.size(); i++) {
            tmp = s[i];
            if (tmp != c) {
                buff += tmp;
            } else {
                if (tmp == c && buff != "") {
                    v.push_back(buff);
                    buff = "";
                }
            }//endif
        }
        if (buff != "") {
            v.push_back(buff);
        }
        return v;
    }

    static std::string createXmlStr(const std::string &uuid, const std::string &domain_name,
                                    int64_t memory, int32_t cpunum, int32_t sockets, int32_t cores, int32_t threads,
                                    const std::string &vedio_pci, const std::string &image_file,
                                    const std::string &disk_file, int32_t vnc_port, const std::string& vnc_pwd) {
        tinyxml2::XMLDocument doc;
        bool is_windows = image_file.find("win") != std::string::npos;

        // <domain>
        tinyxml2::XMLElement *root = doc.NewElement("domain");
        root->SetAttribute("type", "kvm");
        root->SetAttribute("xmlns:qemu", "http://libvirt.org/schemas/domain/qemu/1.0");
        doc.InsertEndChild(root);

        tinyxml2::XMLElement *name_node = doc.NewElement("name");
        name_node->SetText(domain_name.c_str());
        root->LinkEndChild(name_node);

        tinyxml2::XMLElement *uuid_node = doc.NewElement("uuid");
        uuid_node->SetText(uuid.c_str());
        root->LinkEndChild(uuid_node);

        // memory
        tinyxml2::XMLElement *max_memory_node = doc.NewElement("memory");
        max_memory_node->SetAttribute("unit", "KiB");
        max_memory_node->SetText(std::to_string(memory).c_str());
        root->LinkEndChild(max_memory_node);

        tinyxml2::XMLElement *memory_node = doc.NewElement("currentMemory");
        memory_node->SetAttribute("unit", "KiB");
        memory_node->SetText(std::to_string(memory).c_str());
        root->LinkEndChild(memory_node);

        // vcpu
        tinyxml2::XMLElement *vcpu_node = doc.NewElement("vcpu");
        vcpu_node->SetAttribute("placement", "static");
        vcpu_node->SetText(std::to_string(cpunum).c_str());
        root->LinkEndChild(vcpu_node);

        // <os>
        tinyxml2::XMLElement *os_node = doc.NewElement("os");
        tinyxml2::XMLElement *os_sub_node = doc.NewElement("type");
        os_sub_node->SetAttribute("arch", "x86_64");
        os_sub_node->SetAttribute("machine", is_windows ? "q35" : "pc-1.2");
        os_sub_node->SetText("hvm");
        os_node->LinkEndChild(os_sub_node);

        if (!is_windows) {
            tinyxml2::XMLElement *os_sub_node2 = doc.NewElement("boot");
            os_sub_node2->SetAttribute("dev", "hd");
            os_node->LinkEndChild(os_sub_node2);

            tinyxml2::XMLElement *os_sub_node3 = doc.NewElement("boot");
            os_sub_node3->SetAttribute("dev", "cdrom");
            os_node->LinkEndChild(os_sub_node3);
        }
        else {
            tinyxml2::XMLElement *os_bootmenu_node = doc.NewElement("bootmenu");
            os_bootmenu_node->SetAttribute("enable", "yes");
            os_node->LinkEndChild(os_bootmenu_node);
        }
        root->LinkEndChild(os_node);

        // <features>
        tinyxml2::XMLElement *features_node = doc.NewElement("features");
        tinyxml2::XMLElement *features_sub_node1 = doc.NewElement("acpi");
        features_node->LinkEndChild(features_sub_node1);
        tinyxml2::XMLElement *features_sub_node2 = doc.NewElement("apic");
        features_node->LinkEndChild(features_sub_node2);

        tinyxml2::XMLElement *features_sub_node3 = doc.NewElement("hyperv");
        tinyxml2::XMLElement *node_relaxed = doc.NewElement("relaxed");
        node_relaxed->SetAttribute("state", "on");
        features_sub_node3->LinkEndChild(node_relaxed);
        tinyxml2::XMLElement *node_vapic = doc.NewElement("vapic");
        node_vapic->SetAttribute("state", "on");
        features_sub_node3->LinkEndChild(node_vapic);
        tinyxml2::XMLElement *node_spinlocks = doc.NewElement("spinlocks");
        node_spinlocks->SetAttribute("state", "on");
        node_spinlocks->SetAttribute("retries", "8191");
        features_sub_node3->LinkEndChild(node_spinlocks);
        tinyxml2::XMLElement *node_vendor_id = doc.NewElement("vendor_id");
        node_vendor_id->SetAttribute("state", "on");
        node_vendor_id->SetAttribute("value", "1234567890ab");
        features_sub_node3->LinkEndChild(node_vendor_id);
        features_node->LinkEndChild(features_sub_node3);

        tinyxml2::XMLElement *features_sub_node4 = doc.NewElement("kvm");
        tinyxml2::XMLElement *node_hidden = doc.NewElement("hidden");
        node_hidden->SetAttribute("state", "on");
        features_sub_node4->LinkEndChild(node_hidden);
        features_node->LinkEndChild(features_sub_node4);

        tinyxml2::XMLElement* node_vmport = doc.NewElement("vmport");
        node_vmport->SetAttribute("state", "off");
        features_node->LinkEndChild(node_vmport);

        tinyxml2::XMLElement* node_ioapic = doc.NewElement("ioapic");
        node_ioapic->SetAttribute("driver", "kvm");
        features_node->LinkEndChild(node_ioapic);

        root->LinkEndChild(features_node);

        // <cpu>
        tinyxml2::XMLElement *cpu_node = doc.NewElement("cpu");
        cpu_node->SetAttribute("mode", "host-passthrough");
        cpu_node->SetAttribute("check", "none");
        tinyxml2::XMLElement *node_topology = doc.NewElement("topology");
        node_topology->SetAttribute("sockets", std::to_string(sockets).c_str());
        node_topology->SetAttribute("cores", std::to_string(cores).c_str());
        node_topology->SetAttribute("threads", std::to_string(threads).c_str());
        cpu_node->LinkEndChild(node_topology);
        tinyxml2::XMLElement *node_cache = doc.NewElement("cache");
        node_cache->SetAttribute("mode", "passthrough");
        cpu_node->LinkEndChild(node_cache);
        root->LinkEndChild(cpu_node);

        tinyxml2::XMLElement *clock_node = doc.NewElement("clock");
        if (!is_windows) {
            clock_node->SetAttribute("offset", "utc");
        }
        else {
            clock_node->SetAttribute("offset", "localtime");
            tinyxml2::XMLElement *clock_rtc_node = doc.NewElement("timer");
            clock_rtc_node->SetAttribute("name", "rtc");
            clock_rtc_node->SetAttribute("tickpolicy", "catchup");
            clock_node->LinkEndChild(clock_rtc_node);
            tinyxml2::XMLElement *clock_pit_node = doc.NewElement("timer");
            clock_pit_node->SetAttribute("name", "pit");
            clock_pit_node->SetAttribute("tickpolicy", "delay");
            clock_node->LinkEndChild(clock_pit_node);
            tinyxml2::XMLElement *clock_hpet_node = doc.NewElement("timer");
            clock_hpet_node->SetAttribute("name", "hpet");
            clock_hpet_node->SetAttribute("present", "no");
            clock_node->LinkEndChild(clock_hpet_node);
            tinyxml2::XMLElement *clock_hypervclock_node = doc.NewElement("timer");
            clock_hypervclock_node->SetAttribute("name", "hypervclock");
            clock_hypervclock_node->SetAttribute("present", "yes");
            clock_node->LinkEndChild(clock_hypervclock_node);
        }
        root->LinkEndChild(clock_node);

        // tinyxml2::XMLElement *on_poweroff_node = doc.NewElement("on_poweroff");
        // on_poweroff_node->SetText("destroy");
        // root->LinkEndChild(on_poweroff_node);
        // tinyxml2::XMLElement *on_reboot_node = doc.NewElement("on_reboot");
        // on_reboot_node->SetText("restart");
        // root->LinkEndChild(on_reboot_node);
        // tinyxml2::XMLElement *on_crash_node = doc.NewElement("on_crash");
        // on_crash_node->SetText("destroy");
        // root->LinkEndChild(on_crash_node);

        // <pm>
        /*
        tinyxml2::XMLElement* node_pm = doc.NewElement("pm");
        tinyxml2::XMLElement* node_suspend_to_mem = doc.NewElement("suspend-to-mem");
        node_suspend_to_mem->SetAttribute("enabled", "on");
        node_pm->LinkEndChild(node_suspend_to_mem);
        tinyxml2::XMLElement* node_suspend_to_disk = doc.NewElement("suspend-to-disk");
        node_suspend_to_disk->SetAttribute("enabled", "on");
        node_pm->LinkEndChild(node_suspend_to_disk);
        root->LinkEndChild(node_pm);
        */

        tinyxml2::XMLElement *dev_node = doc.NewElement("devices");
        /*
        tinyxml2::XMLElement* vedio_node = doc.NewElement("video");
        tinyxml2::XMLElement* model_node = doc.NewElement("model");
        model_node->SetAttribute("type", "vga");
        model_node->SetAttribute("vram", "16384");
        model_node->SetAttribute("heads", "1");
        vedio_node->LinkEndChild(model_node);
        dev_node->LinkEndChild(vedio_node);
        */

        if (vedio_pci != "") {
            std::vector<std::string> vedios = SplitStr(vedio_pci, '|');
            for (int i = 0; i < vedios.size(); ++i) {
                std::vector<std::string> infos = SplitStr(vedios[i], ':');
                if (infos.size() != 2) {
                    std::cout << vedios[i] << "  error" << std::endl;
                    continue;
                }

                std::vector<std::string> infos2 = SplitStr(infos[1], '.');
                if (infos2.size() != 2) {
                    std::cout << vedios[i] << "  error" << std::endl;
                    continue;
                }

                tinyxml2::XMLElement *hostdev_node = doc.NewElement("hostdev");
                hostdev_node->SetAttribute("mode", "subsystem");
                hostdev_node->SetAttribute("type", "pci");
                hostdev_node->SetAttribute("managed", "yes");

                tinyxml2::XMLElement *source_node = doc.NewElement("source");
                tinyxml2::XMLElement *address_node = doc.NewElement("address");
                address_node->SetAttribute("type", "pci");
                address_node->SetAttribute("domain", "0x0000");
                address_node->SetAttribute("bus", ("0x" + infos[0]).c_str());
                address_node->SetAttribute("slot", ("0x" + infos2[0]).c_str());
                address_node->SetAttribute("function", ("0x" + infos2[1]).c_str());
                source_node->LinkEndChild(address_node);

                hostdev_node->LinkEndChild(source_node);
                dev_node->LinkEndChild(hostdev_node);
            }
        }

        // disk (image)
        tinyxml2::XMLElement *image_node = doc.NewElement("disk");
        image_node->SetAttribute("type", "file");
        image_node->SetAttribute("device", "disk");

        tinyxml2::XMLElement *driver_node = doc.NewElement("driver");
        driver_node->SetAttribute("name", "qemu");
        driver_node->SetAttribute("type", "qcow2");
        image_node->LinkEndChild(driver_node);

        tinyxml2::XMLElement *source_node = doc.NewElement("source");
        source_node->SetAttribute("file", image_file.c_str());
        image_node->LinkEndChild(source_node);

        tinyxml2::XMLElement *target_node = doc.NewElement("target");
        target_node->SetAttribute("dev", is_windows ? "vda" : "hda");
        target_node->SetAttribute("bus", is_windows ? "virtio" : "ide");
        image_node->LinkEndChild(target_node);

        if (is_windows) {
            // set boot order
            tinyxml2::XMLElement *boot_order_node = doc.NewElement("boot");
            boot_order_node->SetAttribute("order", "1");
            image_node->LinkEndChild(boot_order_node);
        }
        dev_node->LinkEndChild(image_node);

        // disk (data)
        tinyxml2::XMLElement *disk_data_node = doc.NewElement("disk");
        disk_data_node->SetAttribute("type", "file");

        tinyxml2::XMLElement *disk_driver_node = doc.NewElement("driver");
        disk_driver_node->SetAttribute("name", "qemu");
        disk_driver_node->SetAttribute("type", "qcow2");
        disk_data_node->LinkEndChild(disk_driver_node);

        tinyxml2::XMLElement *disk_source_node = doc.NewElement("source");
        disk_source_node->SetAttribute("file", disk_file.c_str());
        disk_data_node->LinkEndChild(disk_source_node);

        tinyxml2::XMLElement *disk_target_node = doc.NewElement("target");
        disk_target_node->SetAttribute("dev", is_windows ? "vdb" : "vda");
        disk_target_node->SetAttribute("bus", "virtio");
        disk_data_node->LinkEndChild(disk_target_node);

        dev_node->LinkEndChild(disk_data_node);

        // qemu_guest_agent
        tinyxml2::XMLElement *agent_node = doc.NewElement("channel");
        agent_node->SetAttribute("type", "unix");
        tinyxml2::XMLElement *agent_source_node = doc.NewElement("source");
        agent_source_node->SetAttribute("mode", "bind");
        agent_source_node->SetAttribute("path", ("/tmp/channel_" + uuid + ".sock").c_str());
        agent_node->LinkEndChild(agent_source_node);
        tinyxml2::XMLElement *agent_target_node = doc.NewElement("target");
        agent_target_node->SetAttribute("type", "virtio");
        agent_target_node->SetAttribute("name", "org.qemu.guest_agent.0");
        agent_node->LinkEndChild(agent_target_node);
        dev_node->LinkEndChild(agent_node);

        // network
        tinyxml2::XMLElement *interface_node = doc.NewElement("interface");
        interface_node->SetAttribute("type", "network");
        tinyxml2::XMLElement *interface_source_node = doc.NewElement("source");
        interface_source_node->SetAttribute("network", "default");
        interface_node->LinkEndChild(interface_source_node);
        tinyxml2::XMLElement *interface_model_node = doc.NewElement("model");
        interface_model_node->SetAttribute("type", "virtio");
        interface_node->LinkEndChild(interface_model_node);
        dev_node->LinkEndChild(interface_node);

        // vnc
        tinyxml2::XMLElement *graphics_node = doc.NewElement("graphics");
        graphics_node->SetAttribute("type", "vnc");
        graphics_node->SetAttribute("port", std::to_string(vnc_port).c_str());
        graphics_node->SetAttribute("autoport", vnc_port == -1 ? "yes" : "no");
        graphics_node->SetAttribute("listen", "0.0.0.0");
        graphics_node->SetAttribute("keymap", "en-us");
        graphics_node->SetAttribute("passwd", vnc_pwd.c_str());
        tinyxml2::XMLElement *listen_node = doc.NewElement("listen");
        listen_node->SetAttribute("type", "address");
        listen_node->SetAttribute("address", "0.0.0.0");
        graphics_node->LinkEndChild(listen_node);
        dev_node->LinkEndChild(graphics_node);

        if (is_windows) {
            tinyxml2::XMLElement *memballoon_node = doc.NewElement("memballoon");
            memballoon_node->SetAttribute("model", "none");
            dev_node->LinkEndChild(memballoon_node);
        }
        root->LinkEndChild(dev_node);

        // cpu (qemu:commandline)
        tinyxml2::XMLElement *node_qemu_commandline = doc.NewElement("qemu:commandline");
        tinyxml2::XMLElement *node_qemu_arg1 = doc.NewElement("qemu:arg");
        node_qemu_arg1->SetAttribute("value", "-cpu");
        node_qemu_commandline->LinkEndChild(node_qemu_arg1);
        tinyxml2::XMLElement *node_qemu_arg2 = doc.NewElement("qemu:arg");
        node_qemu_arg2->SetAttribute("value", "host,kvm=off,hv_vendor_id=null");
        node_qemu_commandline->LinkEndChild(node_qemu_arg2);
        tinyxml2::XMLElement *node_qemu_arg3 = doc.NewElement("qemu:arg");
        node_qemu_arg3->SetAttribute("value", "-machine");
        node_qemu_commandline->LinkEndChild(node_qemu_arg3);
        tinyxml2::XMLElement *node_qemu_arg4 = doc.NewElement("qemu:arg");
        node_qemu_arg4->SetAttribute("value", "kernel_irqchip=on");
        node_qemu_commandline->LinkEndChild(node_qemu_arg4);
        root->LinkEndChild(node_qemu_commandline);

        doc.SaveFile("domain_test.xml");
        tinyxml2::XMLPrinter printer;
        doc.Print(&printer);
        return printer.CStr();
    }

    static void printLastError() {
        virErrorPtr error = virGetLastError();
        if (error) {
            std::cout << "vir error: " << error->message << std::endl;
            virFreeError(error);
        }
    }

    std::string preCreateDomain(const std::string &domain_name, const std::string &image_name,
                         int32_t sockets, int32_t cores_per_socket, int32_t threads_per_core,
                         const std::string& vga_pci, int64_t mem, int32_t vnc_port, const std::string& vnc_pwd) {
        bool ret = true;

        std::cout << "domain_name: " << domain_name << std::endl;
        std::cout << "image: " << image_name << std::endl;

        // cpu
        long cpuNumTotal = sockets * cores_per_socket * threads_per_core;
        std::cout << "cpu: " << cpuNumTotal << "(" << sockets << ", " << cores_per_socket << ", " << threads_per_core
                  << ")" << std::endl;

        // gpu
        std::cout << "gpu: " << vga_pci << std::endl;

        // mem
        uint64_t memoryTotal = mem > 1000000000 ? 1000000000 : mem; // KB
        std::cout << "mem: " << memoryTotal << "KB" << std::endl;

        // uuid
        uuid_t uu;
        char buf_uuid[1024] = {0};
        uuid_generate(uu);
        uuid_unparse(uu, buf_uuid);
        std::cout << "uuid: " << buf_uuid << std::endl;

        // 复制一份镜像（系统盘）
        std::string from_image_path = "/data/" + image_name;
        auto pos = image_name.find('.');
        std::string to_image_name = image_name;
        std::string to_ext;
        if (pos != std::string::npos) {
            to_image_name = image_name.substr(0, pos);
            to_ext = image_name.substr(pos + 1);
        }
        std::string to_image_path = "/data/" + to_image_name + "_" + domain_name + "." + to_ext;
        std::cout << "image_file: " << to_image_path << std::endl;
        boost::filesystem::copy_file(from_image_path, to_image_path);

        // 创建虚拟磁盘（数据盘）
        std::string data_file = "/data/data_1_" + domain_name + ".qcow2";
        int64_t disk_total_size = SystemResourceMgr::instance().GetDisk().total / 1024; // GB
        disk_total_size = (disk_total_size - 350) * 0.1;
        std::cout << "data_file: " << data_file << std::endl;
        std::string cmd_create_img =
                "qemu-img create -f qcow2 " + data_file + " " + std::to_string(disk_total_size) + "G";
        std::cout << "create qcow2 image(data): " << cmd_create_img << std::endl;
        std::string create_ret = run_shell(cmd_create_img.c_str());
        std::cout << create_ret << std::endl;

        std::string xml_content = createXmlStr(buf_uuid, domain_name, memoryTotal,
                                               cpuNumTotal, sockets, cores_per_socket, threads_per_core,
                                               vga_pci, to_image_path, data_file, vnc_port, vnc_pwd);
        return xml_content;
    }

    //////////////////////////////////////////////////////////////////////////////////
    void transform_port(const std::string &public_ip, const std::string &transform_port,
                        const std::string &vm_local_ip) {
        std::string cmd;
        cmd += "sudo iptables -t nat -I PREROUTING --protocol tcp --destination " + public_ip +
                " --destination-port " + transform_port + " --jump DNAT --to-destination " + vm_local_ip + ":22";
        cmd += " && sudo iptables -t nat -I PREROUTING -p tcp --dport " + transform_port +
                " -j DNAT --to-destination " + vm_local_ip + ":22";
        auto pos = vm_local_ip.rfind('.');
        std::string ip = vm_local_ip.substr(0, pos) + ".1";
        cmd += " && sudo iptables -t nat -I POSTROUTING -p tcp --dport " + transform_port + " -d " + vm_local_ip +
                " -j SNAT --to " + ip;
        cmd += " && sudo iptables -t nat -I PREROUTING -p tcp -m tcp --dport 20000:60000 -j DNAT --to-destination " +
                vm_local_ip + ":20000-60000";
        cmd += " && sudo iptables -t nat -I PREROUTING -p udp -m udp --dport 20000:60000 -j DNAT --to-destination " +
                vm_local_ip + ":20000-60000";
        cmd += " && sudo iptables -t nat -I POSTROUTING -d " + vm_local_ip +
                " -p tcp -m tcp --dport 20000:60000 -j SNAT --to-source " + public_ip;
        cmd += " && sudo iptables -t nat -I POSTROUTING -d " + vm_local_ip +
                " -p udp -m udp --dport 20000:60000 -j SNAT --to-source " + public_ip;

        run_shell(cmd.c_str());
    }

    void delete_image_file(const std::string &task_id, const std::string& image_name) {
        std::string image = image_name;
        auto pos = image.find('.');
        std::string real_image_name = image;
        std::string ext;
        if (pos != std::string::npos) {
            real_image_name = image.substr(0, pos);
            ext = image.substr(pos + 1);
        }
        std::string real_image_path = "/data/" + real_image_name + "_" + task_id + "." + ext;
        std::cout << "delete image file: " << real_image_path << std::endl;
        remove(real_image_path.c_str());
    }

    void delete_disk_file(const std::string &task_id) {
        std::string disk_file = "/data/data_1_" + task_id + ".qcow2";
        std::cout << "delete disk file: " << disk_file << std::endl;
        remove(disk_file.c_str());
    }

    void delete_iptable(const std::string &public_ip, const std::string &transform_port,
                        const std::string &vm_local_ip) {
        std::string cmd;
        cmd += "sudo iptables -t nat -D PREROUTING --protocol tcp --destination " + public_ip +
                " --destination-port " + transform_port + " --jump DNAT --to-destination " + vm_local_ip + ":22";
        cmd += " && sudo iptables -t nat -D PREROUTING -p tcp --dport " + transform_port +
                " -j DNAT --to-destination " + vm_local_ip + ":22";
        auto pos = vm_local_ip.rfind('.');
        std::string ip = vm_local_ip.substr(0, pos) + ".1";
        cmd += " && sudo iptables -t nat -D POSTROUTING -p tcp --dport " + transform_port + " -d " + vm_local_ip +
                " -j SNAT --to " + ip;
        cmd += " && sudo iptables -t nat -D PREROUTING -p tcp -m tcp --dport 20000:60000 -j DNAT --to-destination " +
                vm_local_ip + ":20000-60000";
        cmd += " && sudo iptables -t nat -D PREROUTING -p udp -m udp --dport 20000:60000 -j DNAT --to-destination " +
                vm_local_ip + ":20000-60000";
        cmd += " && sudo iptables -t nat -D POSTROUTING -d " + vm_local_ip +
                " -p tcp -m tcp --dport 20000:60000 -j SNAT --to-source " + public_ip;
        cmd += " && sudo iptables -t nat -D POSTROUTING -d " + vm_local_ip +
                " -p udp -m udp --dport 20000:60000 -j SNAT --to-source " + public_ip;

        run_shell(cmd.c_str());

        std::cout << "delete iptable: " << public_ip << ", " << transform_port << ", " << vm_local_ip << std::endl;
    }


    void test_kvm(int argc, char** argv) {
        std::string domain_name = "domain_test";
        std::string image_name = "ubuntu.qcow2";

        std::string ssh_port = "6789";

        std::string vm_user = "dbc";
        std::string vm_pwd = "vm123456";

        int32_t vnc_port = -1;
        std::string vnc_pwd = "dbtu@supper2017";

        const ::DeviceCpu& cpus = SystemResourceMgr::instance().GetCpu();
        int sockets = cpus.sockets;
        int cores = cpus.cores_per_socket - 1;
        int threads = cpus.threads_per_core;

        struct sysinfo info{};
        sysinfo(&info);
        int64_t memory_total = info.totalram/1024;
        memory_total *= 0.1; //-= 32 * 1024 * 1024;

        std::string cmd = "lspci |grep NVIDIA |grep -E 'VGA|Audio|USB|Serial bus' | awk '{print $1}' |tr \"\n\" \"|\"";
        std::string vga_gpu = run_shell(cmd.c_str());

        std::string public_ip = get_public_ip();
        if (public_ip.empty()) {
            std::cout << "public_ip is empty" << std::endl;
            return;
        }
        std::cout << "public_ip: " << public_ip << std::endl;

        {
            boost::program_options::options_description opts("dbc command options");
            opts.add_options()
            ("localip", boost::program_options::value<std::string>(), "")
            ("image", boost::program_options::value<std::string>(), "")
            ("vnc_port", boost::program_options::value<int32_t>(), "")
            ("vnc_pwd", boost::program_options::value<std::string>(), "");

            try {
                boost::program_options::variables_map vm;
                boost::program_options::store(boost::program_options::parse_command_line(argc, argv, opts), vm);
                boost::program_options::notify(vm);

                if (vm.count("localip")) {
                    // delete_domain(domain_name, image_name, public_ip, ssh_port, vm["localip"].as<std::string>());
                    return;
                }
                if (vm.count("image")) {
                    image_name = vm["image"].as<std::string>();
                }
                if (vm.count("vnc_port")) {
                    int32_t port = vm["vnc_port"].as<int32_t>();
                    if (port < 5900 || port > 6000) {
                        std::cout << "vnc port should between 5900 and 6000" << std::endl;
                        return;
                    }
                    vnc_port = port;
                }
                if (vm.count("vnc_pwd")) {
                    vnc_pwd = vm["vnc_pwd"].as<std::string>();
                }
            }
            catch (const std::exception &e) {
                std::cout << "invalid command option " << e.what() << std::endl;
                std::cout << opts;
                return;
            }
        }

        boost::filesystem::path image_path("/data/" + image_name);
        boost::system::error_code error_code;
        if (!boost::filesystem::exists(image_path, error_code) || error_code) {
            std::cout << "image does not exist" << std::endl;
            return;
        }
        if (!boost::filesystem::is_regular_file(image_path, error_code) || error_code) {
            std::cout << "image is not a regular file" << std::endl;
            return ;
        }

        try {
            virTool virt;
            if (!virt.openConnect(qemu_url)) {
                printLastError();
                std::cout << "open connect failed" << std::endl;
                print_red("check vm %s failed", domain_name.c_str());
                return;
            }
            {
                std::shared_ptr<virDomainImpl> domain = virt.openDomain(domain_name.c_str());
                if (domain) {
                    domain->deleteDomain();
                    // delete_iptable(public_ip, ssh_port, vm_local_ip);
                }
                std::string xml_content = preCreateDomain(domain_name, image_name, sockets, cores, threads,
                                vga_gpu, memory_total, vnc_port, vnc_pwd);
                sleep(3);
                domain = virt.defineDomain(xml_content.c_str());
                if (!domain) {
                    printLastError();
                    delete_image_file(domain_name, image_name);
                    delete_disk_file(domain_name);
                    print_red("define vm %s failed", domain_name.c_str());
                    print_red("check vm %s failed", domain_name.c_str());
                    return;
                }
                sleep(3);
                if (domain->startDomain() < 0) {
                    printLastError();
                    domain->deleteDomain();
                    print_red("create vm %s failed", domain_name.c_str());
                    print_red("check vm %s failed", domain_name.c_str());
                    return;
                }
                std::string vm_local_ip;
                int32_t try_count = 0;
                while(vm_local_ip.empty() && try_count++ < 100) {
                    if (try_count == 40) {
                        // domain->rebootDomain(VIR_DOMAIN_REBOOT_ACPI_POWER_BTN | VIR_DOMAIN_REBOOT_GUEST_AGENT |
                        // VIR_DOMAIN_REBOOT_INITCTL | VIR_DOMAIN_REBOOT_SIGNAL | VIR_DOMAIN_REBOOT_PARAVIRT);
                        // std::string tmp = run_shell(("virsh reboot --mode agent " + domain_name).c_str());
                        // std::cout << "virsh reboot --mode agent " << domain_name << " return:" << tmp << std::endl;
                    }
                    sleep(3);
                    std::cout << "get vm_local_ip try_count: " << try_count << std::endl;
                    if (domain->getDomainInterfaceAddress(vm_local_ip) < 0) {
                        printLastError();
                    }
                }
                if (image_name.find("win") == std::string::npos) {
                    if (vm_local_ip.empty()) {
                        domain->deleteDomain();
                        std::cout << "get vm_local_ip is empty" << std::endl;
                        print_red("check vm %s failed", domain_name.c_str());
                        return;
                    }
                    std::cout << "vm local ip:" << vm_local_ip << std::endl;
                    transform_port(public_ip, ssh_port, vm_local_ip);
                    try_count = 0;
                    int succ = -1;
                    while (succ < 0 && try_count++ < 100) {
                        sleep(3);
                        std::cout << "set_vm_password try_count: " << try_count << std::endl;
                        succ = domain->setDomainUserPassword(vm_user.c_str(), vm_pwd.c_str());
                        if (succ < 0) {
                            printLastError();
                        }
                    }
                    if (succ < 0) {
                        std::cout << "set_vm_password failed" << std::endl;
                        domain->deleteDomain();
                        delete_iptable(public_ip, ssh_port, vm_local_ip);
                        print_red("check vm %s failed", domain_name.c_str());
                        return;
                    }
                    std::cout << "set vm password successful, user:" << vm_user << ", pwd:" << vm_pwd << std::endl;
                }
                else {
                    std::cout << "windows cannot get local ip and set user password now, please connect vm using vnc" << std::endl;
                }
                std::cout << "vnc port:" << vnc_port << " , password" << vnc_pwd << std::endl;
                print_green("check vm %s successful", domain_name.c_str());
            }
        } catch (...) {
        }
    }
}