/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <string.h>
#include <jni.h>

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include <unistd.h>
#include <sys/inotify.h>

/* 宏定义begin */
//清0宏
#define MEM_ZERO(pDest, destSize) memset(pDest, 0, destSize)

/**
 * 将Java中的String转换成c中的char字符串
 */
char* Jstring2CStr(JNIEnv* env, jstring jstr) {
	char* rtn = NULL;
	jclass clsstring = (*env)->FindClass(env, "java/lang/String"); //String
	jstring strencode = (*env)->NewStringUTF(env, "GB2312"); // 得到一个java字符串 "GB2312"
	jmethodID mid = (*env)->GetMethodID(env, clsstring, "getBytes",
			"(Ljava/lang/String;)[B"); //[ String.getBytes("gb2312");
	jbyteArray barr = (jbyteArray)(*env)->CallObjectMethod(env, jstr, mid,
			strencode); // String .getByte("GB2312");
	jsize alen = (*env)->GetArrayLength(env, barr); // byte数组的长度
	jbyte* ba = (*env)->GetByteArrayElements(env, barr, JNI_FALSE);
	if (alen > 0) {
		rtn = (char*) malloc(alen + 1); //""
		memcpy(rtn, ba, alen);
		rtn[alen] = 0;
	}
	(*env)->ReleaseByteArrayElements(env, barr, ba, 0); //
	return rtn;
}

void Java_com_charon_uninstallfeedback_MainActivity_initUninstallFeedback(
		JNIEnv* env, jobject thiz, jstring packageDir, jint sdkVersion) {

	char * pd = Jstring2CStr(env, packageDir);

	//fork子进程，以执行轮询任务
	pid_t pid = fork();

	if (pid < 0) {
		// fork失败了
	} else if (pid == 0) {
		// 可以一直采用一直判断文件是否存在的方式去判断，但是这样效率稍低，下面使用监听的方式，死循环，每个一秒判断一次，这样太浪费资源了。
//		int check = 1;
//		while (check) {
//			FILE* file = fopen(pd, "rt");
//			if (file == NULL) {
//				if (sdkVersion >= 17) {
//					// Android4.2系统之后支持多用户操作，所以得指定用户
//					execlp("am", "am", "start", "--user", "0", "-a",
//							"android.intent.action.VIEW", "-d",
//							"http://shouji.360.cn/web/uninstall/uninstall.html",
//							(char*) NULL);
//				} else {
//					// Android4.2以前的版本无需指定用户
//					execlp("am", "am", "start", "-a",
//							"android.intent.action.VIEW", "-d",
//							"http://shouji.360.cn/web/uninstall/uninstall.html",
//							(char*) NULL);
//				}
//				check = 0;
//			} else {
//			}
//			sleep(1);
//		}

		// 大神改进的版本，子进程注册"/data/data/packagename"目录监听器，膜拜大神zealotrouge
		// 大神改成使用FileObserver类，这个类用于监听某个文件的变化状态，如果是目录，这个类还可以监听其子目录及子目录文件的变化状态，
		// 通过阅读FileObserver源码，发现其实现利用了Linux内核中一个重要的机制inotify，它是一个内核用于通知用户空间程序文件系统变化的机制，
		// 详情可参考http://en.wikipedia.org/wiki/Inotify，里面对inotify有比较详细的说明。
		// 使用inotify的好处就在于不需要每1s的轮询，这样就不会无谓地消耗系统资源，使用inotify时会用read()方法阻塞进程，
		// 直到收到IN_DELETE通知，此时进程重新被唤醒，执行反馈处理流程。
		int fileDescriptor = inotify_init();
		if (fileDescriptor < 0) {
			// error
			exit(1);
		}

		int watchDescriptor;
		watchDescriptor = inotify_add_watch(fileDescriptor, pd, IN_DELETE);
		if (watchDescriptor < 0) {
			// error
			exit(1);
		}

		//分配缓存，以便读取event，缓存大小=一个struct inotify_event的大小，这样一次处理一个event
		void *p_buf = malloc(sizeof(struct inotify_event));
		if (p_buf == NULL) {
			// error
			exit(1);
		}
		//开始监听
		size_t readBytes = read(fileDescriptor, p_buf,
				sizeof(struct inotify_event));

		//read会阻塞进程，走到这里说明收到目录被删除的事件，注销监听器
		free(p_buf);
		inotify_rm_watch(fileDescriptor, IN_DELETE);

		//目录不存在log
		if (sdkVersion >= 17) {
			execlp("am", "am", "start", "--user", "0", "-a",
					"android.intent.action.VIEW", "-d",
					"http://shouji.360.cn/web/uninstall/uninstall.html",
					(char *) NULL);
		} else {
			execlp("am", "am", "start", "-a", "android.intent.action.VIEW",
					"-d", "http://shouji.360.cn/web/uninstall/uninstall.html",
					(char *) NULL);
		}
	} else {
		//父进程直接退出，使子进程被init进程领养，以避免子进程僵死
	}
}
