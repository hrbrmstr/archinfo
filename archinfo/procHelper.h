//
//  procHelper.h
//  RSwitch
//
//  Created by hrbrmstr on 8/25/19.
//  Copyright © 2019 Bob Rudis. All rights reserved.
//

#ifndef procHelper_h
#define procHelper_h

#include <stdio.h>
#import <Foundation/Foundation.h>

NSString* archInfo(pid_t pid);
NSArray* allProcesses(void);
NSString *processListToJSON(NSArray * processListDict);

#endif /* procHelper_h */
