#import <Cocoa/Cocoa.h>

#include <cmath>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../cpp/src/metrics.h"
#include "../cpp/src/scheduling_algorithms.h"

struct GuiProcess {
  int process_id;
  long long arrival_time;
  long long burst_time;
};

static NSColor* colorForPid(int pid) {
  // Deterministic bright-ish color from PID.
  // Keep it simple: map pid to hue.
  CGFloat hue = fmod(static_cast<CGFloat>(pid) * 37.0f, 360.0f) / 360.0f;
  return [NSColor colorWithHue:hue saturation:0.75f brightness:0.95f alpha:1.0f];
}

@interface TimelineView : NSView
{
@public
  std::vector<ProcessSlice> timeline_;
}
@end

@implementation TimelineView
- (void)drawRect:(NSRect)dirtyRect {
  [super drawRect:dirtyRect];

  NSRect b = self.bounds;
  long long total = 0;
  for (const auto& s : timeline_) total += s.burst_time;

  if (timeline_.empty() || total <= 0) {
    NSString* placeholder = @"Run a simulation to see the timeline";
    NSDictionary* attrs = @{NSFontAttributeName : [NSFont systemFontOfSize:13],
                            NSForegroundColorAttributeName : [NSColor secondaryLabelColor]};
    [placeholder drawInRect:NSMakeRect(10, b.size.height / 2 - 8, b.size.width - 20, 16) withAttributes:attrs];
    return;
  }

  const CGFloat marginX = 10;
  const CGFloat rectY = b.size.height - 90;
  const CGFloat rectH = 60;
  const CGFloat availableW = std::max(1.0, b.size.width - 2 * marginX);

  // Draw slices
  CGFloat cursorX = marginX;
  long long t = 0;
  for (size_t i = 0; i < timeline_.size(); i++) {
    const auto& s = timeline_[i];
    double frac = static_cast<double>(s.burst_time) / static_cast<double>(total);
    CGFloat sliceW = static_cast<CGFloat>(std::round(frac * availableW));
    sliceW = std::max(1.0, sliceW);

    NSColor* fill = nil;
    NSString* label = nil;
    if (s.process_id == -1) {
      fill = [NSColor colorWithCalibratedWhite:0.85 alpha:1.0];
      label = @"Idle";
    } else {
      fill = colorForPid(s.process_id);
      label = [NSString stringWithFormat:@"P%d", s.process_id];
    }

    [fill setFill];
    [[NSBezierPath bezierPathWithRect:NSMakeRect(cursorX, rectY, sliceW, rectH)] fill];

    NSDictionary* textAttrs = @{
      NSFontAttributeName : [NSFont systemFontOfSize:10],
      NSForegroundColorAttributeName : [NSColor whiteColor]
    };
    // Center text in the rectangle
    NSSize textSize = [label sizeWithAttributes:textAttrs];
    CGFloat tx = cursorX + (sliceW - textSize.width) / 2.0;
    CGFloat ty = rectY + (rectH - textSize.height) / 2.0;
    [label drawAtPoint:NSMakePoint(tx, ty) withAttributes:textAttrs];

    cursorX += sliceW;
    t += s.burst_time;
  }

  // Draw tick labels at slice boundaries
  const CGFloat tickY = rectY - 18;
  NSDictionary* tickAttrs = @{
    NSFontAttributeName : [NSFont systemFontOfSize:9],
    NSForegroundColorAttributeName : [NSColor secondaryLabelColor]
  };

  long long cumulative = 0;
  CGFloat x = marginX;
  for (size_t i = 0; i < timeline_.size(); i++) {
    NSString* ts = [NSString stringWithFormat:@"%lld", cumulative];
    NSSize tsSize = [ts sizeWithAttributes:tickAttrs];
    CGFloat tx = x;
    [ts drawInRect:NSMakeRect(tx, tickY, tsSize.width, tsSize.height) withAttributes:tickAttrs];

    const auto& s = timeline_[i];
    double frac = static_cast<double>(s.burst_time) / static_cast<double>(total);
    CGFloat sliceW = static_cast<CGFloat>(std::round(frac * availableW));
    sliceW = std::max(1.0, sliceW);
    cumulative += s.burst_time;
    x += sliceW;
  }
  NSString* endTs = [NSString stringWithFormat:@"%lld", cumulative];
  [endTs drawInRect:NSMakeRect(x, tickY, 40, 12) withAttributes:tickAttrs];
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate {
  NSWindow* window_;
  NSPopUpButton* algoPopup_;
  NSTextField* quantumField_;
  NSTextField* arrivalField_;
  NSTextField* burstField_;
  NSTextView* processesView_;
  NSTextView* metricsView_;
  TimelineView* timelineView_;
  std::vector<GuiProcess> processes_;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
  NSRect frame = NSMakeRect(0, 0, 980, 650);
  window_ = [[NSWindow alloc] initWithContentRect:frame
                                        styleMask:(NSWindowStyleMaskTitled |
                                                   NSWindowStyleMaskClosable |
                                                   NSWindowStyleMaskResizable)
                                          backing:NSBackingStoreBuffered
                                            defer:NO];
  [window_ setTitle:@"Scheduling Algorithm Simulator (C++ native GUI)"];

  NSView* content = window_.contentView;

  CGFloat pad = 14;
  CGFloat y = frame.size.height - pad;

  // Algorithm row
  algoPopup_ = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(pad, y - 24, 220, 26)];
  [algoPopup_ addItemsWithTitles:@[@"FCFS", @"RR", @"SJF", @"SRTF"]];
  [algoPopup_ selectItemAtIndex:0];
  [content addSubview:algoPopup_];

  NSTextField* algoLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(pad, y - 42, 90, 18)];
  [algoLabel setStringValue:@"Algorithm"];
  [algoLabel setBezeled:NO];
  [algoLabel setDrawsBackground:NO];
  [algoLabel setEditable:NO];
  [algoLabel setSelectable:NO];
  [content addSubview:algoLabel];

  quantumField_ = [[NSTextField alloc] initWithFrame:NSMakeRect(pad + 240, y - 24, 80, 26)];
  [quantumField_ setStringValue:@"2"];
  [content addSubview:quantumField_];

  NSTextField* qLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(pad + 240, y - 42, 80, 18)];
  [qLabel setStringValue:@"Quantum"];
  [qLabel setBezeled:NO];
  [qLabel setDrawsBackground:NO];
  [qLabel setEditable:NO];
  [qLabel setSelectable:NO];
  [content addSubview:qLabel];

  y -= 60;

  // Process input
  arrivalField_ = [[NSTextField alloc] initWithFrame:NSMakeRect(pad, y - 24, 80, 26)];
  [arrivalField_ setStringValue:@"0"];
  [content addSubview:arrivalField_];

  NSTextField* aLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(pad, y - 42, 80, 18)];
  [aLabel setStringValue:@"Arrival"];
  [aLabel setBezeled:NO];
  [aLabel setDrawsBackground:NO];
  [aLabel setEditable:NO];
  [aLabel setSelectable:NO];
  [content addSubview:aLabel];

  burstField_ = [[NSTextField alloc] initWithFrame:NSMakeRect(pad + 100, y - 24, 80, 26)];
  [burstField_ setStringValue:@"1"];
  [content addSubview:burstField_];

  NSTextField* bLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(pad + 100, y - 42, 80, 18)];
  [bLabel setStringValue:@"Burst"];
  [bLabel setBezeled:NO];
  [bLabel setDrawsBackground:NO];
  [bLabel setEditable:NO];
  [bLabel setSelectable:NO];
  [content addSubview:bLabel];

  NSButton* addBtn = [[NSButton alloc] initWithFrame:NSMakeRect(pad + 210, y - 26, 120, 28)];
  [addBtn setTitle:@"Add Process"];
  [addBtn setTarget:self];
  [addBtn setAction:@selector(onAddProcess:)];
  [content addSubview:addBtn];

  NSButton* clearBtn = [[NSButton alloc] initWithFrame:NSMakeRect(pad + 340, y - 26, 95, 28)];
  [clearBtn setTitle:@"Clear All"];
  [clearBtn setTarget:self];
  [clearBtn setAction:@selector(onClearAll:)];
  [content addSubview:clearBtn];

  NSButton* runBtn = [[NSButton alloc] initWithFrame:NSMakeRect(pad + 450, y - 26, 90, 28)];
  [runBtn setTitle:@"Run"];
  [runBtn setTarget:self];
  [runBtn setAction:@selector(onRun:)];
  [content addSubview:runBtn];

  y -= 68;

  // Timeline
  timelineView_ = [[TimelineView alloc] initWithFrame:NSMakeRect(pad, y - 130, frame.size.width - 2 * pad, 130)];
  timelineView_.wantsLayer = YES;
  timelineView_.layer.backgroundColor = [[NSColor windowBackgroundColor] CGColor];
  [content addSubview:timelineView_];

  y -= 145;

  // Two text areas: processes and metrics
  CGFloat halfW = (frame.size.width - 2 * pad - 18) / 2.0;
  NSScrollView* processesScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(pad, pad + 60, halfW, 90)];
  processesScroll.hasVerticalScroller = YES;

  processesView_ = [[NSTextView alloc] initWithFrame:processesScroll.contentView.bounds];
  [processesView_ setEditable:NO];
  processesScroll.documentView = processesView_;
  [content addSubview:processesScroll];

  NSTextField* pLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(pad, pad + 150, halfW, 18)];
  [pLabel setStringValue:@"Processes"];
  [pLabel setBezeled:NO];
  [pLabel setDrawsBackground:NO];
  [pLabel setEditable:NO];
  [pLabel setSelectable:NO];
  [content addSubview:pLabel];

  NSScrollView* metricsScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(pad + halfW + 18, pad + 60, halfW, 90)];
  metricsScroll.hasVerticalScroller = YES;

  metricsView_ = [[NSTextView alloc] initWithFrame:metricsScroll.contentView.bounds];
  [metricsView_ setEditable:NO];
  metricsScroll.documentView = metricsView_;
  [content addSubview:metricsScroll];

  NSTextField* mLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(pad + halfW + 18, pad + 150, halfW, 18)];
  [mLabel setStringValue:@"Metrics"];
  [mLabel setBezeled:NO];
  [mLabel setDrawsBackground:NO];
  [mLabel setEditable:NO];
  [mLabel setSelectable:NO];
  [content addSubview:mLabel];

  [processesView_ setString:@"(none)"];
  [metricsView_ setString:@""];

  [window_ makeKeyAndOrderFront:nil];
}

- (void)onAddProcess:(id)sender {
  (void)sender;

  bool okArrival = false;
  bool okBurst = false;
  long long arrival = [arrivalField_.stringValue longLongValue];
  long long burst = [burstField_.stringValue longLongValue];
  okArrival = true;
  okBurst = true;

  if (arrival < 0 || arrival > 100) {
    NSBeep();
    return;
  }
  if (burst <= 0 || burst > 100) {
    NSBeep();
    return;
  }

  GuiProcess gp;
  gp.process_id = static_cast<int>(processes_.size()) + 1;
  gp.arrival_time = arrival;
  gp.burst_time = burst;
  processes_.push_back(gp);

  std::ostringstream oss;
  oss << "PID\\tArrival\\tBurst\\n";
  for (const auto& p : processes_) {
    oss << p.process_id << "\\t" << p.arrival_time << "\\t" << p.burst_time << "\\n";
  }
  NSString* out = [NSString stringWithUTF8String:oss.str().c_str()];
  [processesView_ setString:out];
}

- (void)onClearAll:(id)sender {
  (void)sender;
  processes_.clear();
  [processesView_ setString:@"(none)"];
  [metricsView_ setString:@""];
  timelineView_->timeline_.clear();
  [timelineView_ setNeedsDisplay:YES];
}

- (void)onRun:(id)sender {
  (void)sender;
  if (processes_.empty()) {
    NSBeep();
    return;
  }

  const NSString* algo = algoPopup_.titleOfSelectedItem;
  const std::string algoStd = [algo UTF8String];
  long long quantum = [quantumField_.stringValue longLongValue];
  if (algoStd == "RR" && quantum <= 0) {
    NSBeep();
    return;
  }

  std::vector<ProcessInput> inputs;
  inputs.reserve(processes_.size());
  for (const auto& gp : processes_) {
    inputs.push_back(ProcessInput{gp.process_id, gp.arrival_time, gp.burst_time});
  }

  std::vector<ProcessSlice> timeline;
  if (algoStd == "FCFS") timeline = firstComeFirstServe(inputs);
  if (algoStd == "SJF") timeline = shortestJobFirst(inputs);
  if (algoStd == "SRTF") timeline = shortestRemainingTimeFirst(inputs);
  if (algoStd == "RR") timeline = roundRobin(inputs, quantum);

  const SummaryMetrics summary = computeSummaryMetrics(algoStd, inputs, timeline);
  auto perProc = computePerProcessMetrics(algoStd, inputs, timeline);

  timelineView_->timeline_ = timeline;
  [timelineView_ setNeedsDisplay:YES];

  std::ostringstream oss;
  oss << "Avg Waiting Time: " << static_cast<double>(summary.avg_waiting_time) << "\\n";
  oss << "Avg Turnaround Time: " << static_cast<double>(summary.avg_turnaround_time) << "\\n";
  oss << "Throughput: " << static_cast<double>(summary.throughput) << "\\n";
  oss << "CPU Utilization: " << static_cast<double>(summary.cpu_utilization_percent) << "%\\n\\n";

  oss << "PID\\tArrival\\tBurst\\tWaiting\\tTurnaround\\n";
  for (const auto& gp : processes_) {
    auto it = perProc.find(gp.process_id);
    long double waiting = (it != perProc.end()) ? it->second.waiting_time : 0;
    long double tat = (it != perProc.end()) ? it->second.turnaround_time : 0;
    oss << gp.process_id << "\\t" << gp.arrival_time << "\\t" << gp.burst_time << "\\t"
        << static_cast<double>(waiting) << "\\t" << static_cast<double>(tat) << "\\n";
  }

  NSString* out = [NSString stringWithUTF8String:oss.str().c_str()];
  [metricsView_ setString:out];
}

@end

int main(int argc, const char* argv[]) {
  @autoreleasepool {
    NSApplication* app = [NSApplication sharedApplication];
    AppDelegate* delegate = [AppDelegate new];
    [app setDelegate:delegate];
    return NSApplicationMain(argc, argv);
  }
}

