#!/usr/bin/env python
import hashlib 
import subprocess
import os
import sys
import shutil
import hashlib
import socket
import fcntl
import struct

#global variables
from http.server import BaseHTTPRequestHandler, HTTPServer
mmcblk1_hash = " "
mmcblk1p9_hash = " "
mmcblk1boot0_hash = " "
mmcblk1boot1_hash = " " 
tsecfw_hash = " "
tsecfw_size = 3840 
tsecfw_offset = " "
get_nand_hash = True

# HTTPRequestHandler class
class HTTPServer_RequestHandler(BaseHTTPRequestHandler):
    global mmcblk1_hash
    global mmcblk1p9_hash
    global mmcblk1boot0_hash
    global mmcblk1boot1_hash
    global tsecfw_size
    global tsecfw_hash
    global tsecfw_offset
    filename = " "
    # GET
    def do_GET(self):
        request_path = self.path
        request_path = request_path.strip('/')
        download = False
        size = 0
        path = None
        if (
            request_path == "mmcblk1" or
            request_path == "mmcblk1boot0" or
            request_path == "mmcblk1boot1" or
	        request_path == "tsec_fw" or
            request_path == "mmcblk1p9"
        ):
            download = True
            if(request_path != "tsec_fw"):
                path = "/dev/%s"%(request_path)
                size = subprocess.run(["blockdev","--getsize64",path],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
                size = size.stdout.decode()
                filename = "%s.bin"%request_path            
            else:
                filename = "tsecfw.inl"
                size = tsecfw_size
        try:
            if(download):
                self.send_response(200)
                self.send_header("Content-Type:","application/octet-stream")
                self.send_header("Content-Disposition:","attachment;filename=%s"%filename)
                self.send_header("Content-Lenght:","%i"%int(size))
                self.end_headers()
                #self.wfile.write(bytes("downloading file...<br>","utf8"))
                #self.wfile.write(file.read())
                if(request_path == "tsec_fw"):
                    tsecfd = open('/dev/mmcblk1boot0','rb')
                    tsecfd.seek(int(tsecfw_offset,0))
                    self.wfile.write(tsecfd.read(tsecfw_size))
                    tsecfd.close()
                else:
                    with open(path, 'rb') as content:
                        shutil.copyfileobj(content, self.wfile)
            else:
                fd = open('index.html','r')
                file_content = fd.read()
                fd.close()

                #boot0 hash
                if(mmcblk1boot0_hash != " "):
                    html = file_content.replace("mmcblk1boot0_hash",mmcblk1boot0_hash)
                else:
                    html = file_content.replace("- MD5 : mmcblk1boot0_hash","")

                #boot1 hash
                if(mmcblk1boot0_hash != " "):
                    html = html.replace("mmcblk1boot1_hash",mmcblk1boot1_hash)
                else:
                    html = html.replace("- MD5 : mmcblk1boot1_hash","")

                #NAND
                if(mmcblk1_hash != " "):
                    html = html.replace("mmcblk1_hash",mmcblk1_hash)
                else:
                    html = html.replace("- MD5 : mmcblk1_hash","")

                #tsec fw
                if(tsecfw_hash != " "):
                    html = html.replace("TSEC_FW_hash",tsecfw_hash)
                else:
                    html = html.replace("- MD5 : TSEC_FW_hash","")

                if(mmcblk1p9_hash != " "):
                    html = html.replace("mmcblk1p9_hash",mmcblk1p9_hash)
                else:
                    html = html.replace("- MD5 : mmcblk1p2","")

                self.send_response(200)
                self.send_header('Content-type','text/html')
                self.end_headers()
                self.wfile.write(html.encode())
        except Exception as e:
            self.send_error(404,"File Not Found: %s" % self.path)
            print('error occured on :')
            print(e) 
        return

def CheckTSECFW():
    global tsecfw_hash
    global tsecfw_offset
    global tsecfw_size
    try:
        file = open('/dev/mmcblk1boot0','rb')
        #i hate doing this as it makes it load the whole file in memory but...
        data = file.read()
        tsecfw_offset = hex(data.index(b'\x4d\x00\x42\xcf'))
        print('detected tsecfw_offset : %s'%tsecfw_offset)
        if(tsecfw_offset == None or tsecfw_offset == " "):
            print('did not find tsec FW!')
            return

        file.seek(int(tsecfw_offset,0))
        tsecfw = file.read(tsecfw_size)
        print('got fw!')
        m = hashlib.md5()
        m.update(tsecfw)
        tsecfw_hash = m.hexdigest()
        print('hash : %s'%tsecfw_hash)
        file.close()
    except Exception as e:
        print('error occured on :')
        print(e) 
    
    return
def calc_hashes():
    global mmcblk1_hash
    global mmcblk1p9_hash
    global mmcblk1boot0_hash
    global mmcblk1boot1_hash
    global get_nand_hash

    if(get_nand_hash):
        output = subprocess.run(["md5sum","/dev/mmcblk1"],stdout=subprocess.PIPE)
        hash = output.stdout.decode('utf-8')
        mmcblk1_hash = hash[:32]
        print('mmcblk1 hash : %s'%mmcblk1_hash)

    output = subprocess.run(["md5sum","/dev/mmcblk1p9"],stdout=subprocess.PIPE)
    hash = output.stdout.decode('utf-8')
    mmcblk1p2_hash = hash[:32]
    print('mmcblk1p2 hash : %s'%mmcblk1p2_hash)


    output = subprocess.run(["md5sum","/dev/mmcblk1boot0"],stdout=subprocess.PIPE)
    hash = output.stdout.decode('utf-8')
    mmcblk1boot0_hash = hash[:32]
    print('boot0 hash : %s'%mmcblk1boot0_hash)
    
    output = subprocess.run(["md5sum","/dev/mmcblk1boot1"],stdout=subprocess.PIPE)
    hash = output.stdout.decode('utf-8')
    mmcblk1boot1_hash = hash[:32]
    print('boot1 hash : %s'%mmcblk1boot1_hash)

def GetIpAddress(ifname):
    #s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    #return socket.inet_ntoa(fcntl.ioctl(
    #    s.fileno(),
    #    0x8915,  # SIOCGIFADDR
    #    struct.pack('256s', ifname[:15])
    #)[20:24])
    output = subprocess.check_output(['hostname', '--ip-address'])
    output = output.decode()
    output = output.replace("\n","")
    output = output.replace(" ","")
    return output

def ShowArguments():
    print('Arguments : ')
    print('\t-s or --skip : skip making hash for the nand, making server boot faster')
    print('\t-h or --help : show this message')   

def run():
    global mmcblk1_hash
    global mmcblk1boot0_hash
    global mmcblk1boot1_hash
    global get_nand_hash

    print('\n\r\n\r')
    print('--------------------DacoTaco presents...------------------------')
    print('\t\tHELLOOOO AND WELCOME TO YASDU!')
    print('\t(Y)et (A)nother (S)witch (D)umping (U)tility')  
    print('----------------------------------------------------------------\n\r\n\r')

    argc = len(sys.argv)
    for i in range(1,argc):
        if(sys.argv[i] == "-s" or sys.argv[i] == "--skip"):
            get_nand_hash = False
        elif(sys.argv[i] == "-h" or sys.argv[i] == "--help"):
            ShowArguments()        
            exit()
        else:
            print("unknown argument %s"%sys.argv[i])
            ShowArguments()
            exit()

    #check if we are root...
    if os.geteuid() != 0:
        exit("You need to have root privileges to run this script.\nDid you use sudo?")

    print('calculating hashes...')

    if(get_nand_hash):
        print('this could take a while ( +/- 7min on 32GB switch) so get a drink :)')
    
    calc_hashes()
    #print("var : '%s'"%mmcblk1boot1_hash)

    print('retrieving FSEC FW...')
    CheckTSECFW()
  
    print('starting server...')
    # Server settings
    # listen to all connections (internal and external) & on port 1337
    server_address = ('0.0.0.0', 1337 )
    httpd = HTTPServer(server_address, HTTPServer_RequestHandler)
    print('running server...')
    print('server ip : http://%s:1337'%GetIpAddress('eth0'))
    httpd.serve_forever()
 
 
run()
