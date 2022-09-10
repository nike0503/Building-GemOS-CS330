#include "wc.h"


extern struct team teams[NUM_TEAMS];
extern int test;
extern int finalTeam1;
extern int finalTeam2;

int processType = HOST;
const char *team_names[] = {
  "India", "Australia", "New Zealand", "Sri Lanka",   // Group A
  "Pakistan", "South Africa", "England", "Bangladesh" // Group B
};

//The error cases only show where the error occurs
void teamPlay(void)
{
  int fd;
  char filename[30];
  char testno[10];
  sprintf(testno, "%d", test);
  strcpy(filename, "test/");
  strcat(filename, testno);
  strcat(filename, "/inp/");
  strcat(filename, teams[processType].name);
  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    perror ("Failed to open input file");
    exit(-1);
  }
  
  //start listening on pipe
  while (1) {
    char msg[2];
    char score[2];
    if (read(teams[processType].commpipe[0], msg, 1) != 1) {
      perror("Unable to read from commpipe");
      exit(-1);
    }
    //read is a blocking function, which means it waits for the msg from the host/group
    //only when the msg is received does it perform the task, i.e. read one score or exit

    //check msg from commpipe
    if (!strcmp(msg, "1")) {
      //read from file if msg is 1 and write to matchpipe
      if (read(fd, score, 1) != 1) {
        perror("Unable to read from team file");
        exit(-1);
      }
      if (write(teams[processType].matchpipe[1], score, 1) != 1) {
        perror("Unable to write from teamPlay to matchpipe");
        exit(-1);
      }
    }
    //else self terminate
    else {
      close(fd);
      exit(0);
    }
  }
}

void endTeam(int teamID)
{
  //msg 0 indicates a call to terminate the file
  if (write(teams[teamID].commpipe[1], "0", 1) != 1) {
    perror("Unable to write endTeam to commpipe");
    exit(-1);
  } 
}

int match(int team1, int team2)
{
  int final = 0;

  //the code ensures that team1 is currently from group1 and team2 from group2
  if (team1 <= 3 && team2 > 3) {
    final = 1;
  }
  int winner;
  //receive the toss result and then create the output file
  if (write(teams[team1].commpipe[1], "1", 1) != 1) {
    perror("Unable to write from match to commpipe for team 1 toss");
    exit(-1);
  }
  if (write(teams[team2].commpipe[1], "1", 1) != 1) {
    perror("Unable to write from match to commpipe for team 2 toss");
  }
  char s1[2];
  char s2[2];
  if (read(teams[team1].matchpipe[0], s1, 1) != 1) {
    perror("Unable to read from match pipe for team1 toss");
    exit(0);
  }
  if (read(teams[team2].matchpipe[0], s2, 1) != 1) {
    perror("Unable to read from match pipe for team2 toss");
    exit(0);
  }
  int score1 = atoi(s1);
  int score2 = atoi(s2);
 
  //if team2 bats first, interchange their ids
  if ((score1 + score2)%2 == 0) {
    int temp = team1;
    team1 = team2;
    team2 = temp;
  }
  char filename[40];
  char testno[10];
  sprintf(testno, "%d", test);
  strcpy(filename, "test/");
  strcat(filename, testno);
  strcat(filename, "/out");

  mkdir(filename, 0775); //if out doesn't exist, creates a new out

  strcat(filename, "/");
  strcat(filename, teams[team1].name);
  strcat(filename, "v");
  strcat(filename, teams[team2].name);
  if (final) {
    strcat(filename, "-Final");
  }
  int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0777);
  if (fd < 0) {
    perror("Failed to open output file");
    exit(-1);
  }

  char outMsg[50];
  int curWkt = 0;
  int curBatsRuns = 0;
  int totalRuns1 = 0; //team1 total
  int totalRuns2 = 0; //team2 total
  int i;

  //team 1
  strcpy(outMsg, "Innings1: ");
  strcat(outMsg, teams[team1].name);
  strcat(outMsg, " bats\n");
  if (write(fd, outMsg, strlen(outMsg)) != strlen(outMsg)) {
    perror("Unable to write Innings1 batting header");
    exit(-1);
  }
  
  for (i = 1; i <= 120; i++) {
    if (write(teams[team1].commpipe[1], "1", 1) != 1) { //the commpipe[0] is already waiting for the msg due to blocking read in teamPlay()
      perror("Unable to write from match to commpipe for batting 1");
      exit(-1);
    }
    if (write(teams[team2].commpipe[1], "1", 1) != 1) {
      perror("Unable to write from match to commpipe for bowling 1");
    }
    if (read(teams[team1].matchpipe[0], s1, 1) != 1) { //it waits for the teamPlay() to write the score in the matchpipe[0]
      perror("Unable to read from match pipe for batting 1");
      exit(0);
    }
    if (read(teams[team2].matchpipe[0], s2, 1) != 1) {
      perror("Unable to read from match pipe for bowling 1");
      exit(0);
    }
    score1 = atoi(s1);
    score2 = atoi(s2);
    if (score1 != score2) {
      curBatsRuns += score1;
      totalRuns1 += score1;
    }
    else {
      //print the runs of the curWkt
      curWkt++;
      sprintf(outMsg, "%d:%d\n", curWkt, curBatsRuns);
      if (write(fd, outMsg, strlen(outMsg)) != strlen(outMsg)) {
        perror("Unable to write wicket runs");
        exit(0);
      }
      curBatsRuns = 0;
      if (curWkt == 10) {
        break;
      }
    }
  }
  //if the team was not allout
  if (curWkt != 10) {
    sprintf(outMsg, "%d:%d*\n", curWkt+1, curBatsRuns);
    if (write(fd, outMsg, strlen(outMsg)) != strlen(outMsg)) {
      perror("Unable to write last not out wicket for batting 1");
      exit(-1);
    }
    curBatsRuns = 0;
  }
  sprintf(outMsg, "%s TOTAL: %d\n", teams[team1].name, totalRuns1);
  if (write(fd, outMsg, strlen(outMsg)) != strlen(outMsg)) {
    perror("Unable to write total for batting team 1");
    exit(-1);
  }

  //team 2
  curWkt = 0;
  strcpy(outMsg, "Innings2: ");
  strcat(outMsg, teams[team2].name);
  strcat(outMsg, " bats\n");
  if (write(fd, outMsg, strlen(outMsg)) != strlen(outMsg)) {
    perror("Unable to write Innings2 batting header");
    exit(-1);
  }
  for (i = 1; i <= 120; i++) {
    if (write(teams[team1].commpipe[1], "1", 1) != 1) {
      perror("Unable to write from match to commpipe for batting 2");
      exit(-1);
    }
    if (write(teams[team2].commpipe[1], "1", 1) != 1) {
      perror("Unable to write from match to commpipe for bowling 2");
      exit(-1);
    }
    if (read(teams[team1].matchpipe[0], s1, 1) != 1) {
      perror("Unable to read from match pipe for batting 2");
      exit(0);
    }
    if (read(teams[team2].matchpipe[0], s2, 1) != 1) {
      perror("Unable to read from match pipe for bowling 2");
      exit(0);
    }
    score1 = atoi(s1);
    score2 = atoi(s2);
    if (score1 != score2) {
      curBatsRuns += score2;
      totalRuns2 += score2;
      //break if the total exceeds the target
      if (totalRuns2 > totalRuns1) {
        break;
      }
    }
    else {
      //print the runs of the curWkt
      curWkt++;
      sprintf(outMsg, "%d:%d\n", curWkt, curBatsRuns);
      if (write(fd, outMsg, strlen(outMsg)) != strlen(outMsg)) {
        perror("Unable to write wicket runs");
        exit(0);
      }
      curBatsRuns = 0;
      if (curWkt == 10) {
        break;
      }
    }
  }
  //if the team was not allout
  if (curWkt != 10) {
    sprintf(outMsg, "%d:%d*\n", curWkt+1, curBatsRuns);
    if (write(fd, outMsg, strlen(outMsg)) != strlen(outMsg)) {
      perror("Unable to write last not out wicket for batting 2");
      exit(-1);
    }
    curBatsRuns = 0;
  }
  sprintf(outMsg, "%s TOTAL: %d\n", teams[team2].name, totalRuns2);
  if (write(fd, outMsg, strlen(outMsg)) != strlen(outMsg)) {
    perror("Unable to write total for batting team 2");
    exit(-1);
  }

  //return the result
  //team2 wins
  if (totalRuns2 > totalRuns1) {
    sprintf(outMsg, "%s beats %s by %d wickets\n", teams[team2].name, teams[team1].name, (10-curWkt));
    if (write(fd, outMsg, strlen(outMsg)) != strlen(outMsg)) {
      perror("Unable to write team2 wins result");
      exit(-1);
    }
    winner = team2;
  }
  //team1 wins
  else if (totalRuns2 < totalRuns1) {
    sprintf(outMsg, "%s beats %s by %d runs\n", teams[team1].name, teams[team2].name, (totalRuns1-totalRuns2));
    if (write(fd, outMsg, strlen(outMsg)) != strlen(outMsg)) {
      perror("Unable to write team1 wins result");
      exit(-1);
    }
    winner = team1;
  }
  //tie
  else {
    //if team2 has lower index, make it team1
    if (team2 < team1) {
      int temp = team2;
      team2 = team1;
      team1 = temp;
    }
    sprintf(outMsg, "TIE: %s beats %s\n", teams[team1].name, teams[team2].name);
    if (write(fd, outMsg, strlen(outMsg)) != strlen(outMsg)) {
      perror("Unable to write tie result");
      exit(-1);
    }
    winner = team1;
  }
	return winner;
}

void spawnTeams(void)
{
  //create the 8 team processes and call teamPlay() from them
  for (int i = 0; i < 8; i++) {
    strcpy(teams[i].name, team_names[i]);
    pipe(teams[i].matchpipe);
    pipe(teams[i].commpipe);
    pid_t team = fork();
    if (!team) {
      processType = i;
      teamPlay();
    }
  }
	return;
}

void conductGroupMatches(void)
{
  pid_t groups[2];
  int grppipe[2][2];
  int winner[2];
  int points[8]; //points table
  for (int i = 0; i < 8; i++) {
    points[i] = 0;
  }
  for (int i = 0; i < 2; i++) { //i = groupNo in this loop
    pipe(grppipe[i]);
    groups[i] = fork();
    if (!groups[i]) {
      //the host spawns the 2 groups, and waits for their winner
      for (int team1 = 4*i; team1 < 4*(i+1)-1; team1++) {
        for (int team2 = team1+1; team2 < 4*(i+1); team2++) {
          int winner = match(team1, team2);
          // printf("group :%d, %d\n",i, winner);
          points[winner]++; //increase the points of the winner
        }
      }

      //determine the leader of the group (members from 4i to 4i+3)
      winner[i] = 4*i;
      // printf("%d %d %d %d %d %d %d %d\n", points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]);
      for (int j = 4*i; j < 4*(i+1); j++) {
        if (points[j] > points[winner[i]]) {
          winner[i] = j;
        }
      }
      printf("Winner %d: %d\n", i, winner[i]);
      //end the teams who are not the winner
      for (int j = 4*i; j < 4*(i+1); j++) {
        if (j != winner[i]) {
          endTeam(j);
        }
      }
      //write the winner id to the pipe
      char win[2];
      sprintf(win, "%d", winner[i]);
      if (write(grppipe[i][1], win, strlen(win)) != strlen(win)) {
        perror("Unable to write group leader from group to host");
        exit(-1);
      }
      //end the group process
      exit(0);
    }
  }
  
  //wait for both the groups to finish
  int status1, status2;
  waitpid(groups[0], &status1, 0);
  waitpid(groups[1], &status2, 0);

  if (status1 == -1) {
    for (int k = 0; k < 4; k++) {
      endTeam(k);
    }
  }
  if (status2 == -1) {
    for (int k = 4; k < 8; k++) {
      endTeam(k);
    }
  }

  if (status1 == -1 || status2 == -1) {
    printf("All matches could not be completed\n");
    exit(-1);
  }
  
  //finalTeam1 = read from grp[0][0]
  char final[2];
  if (read(grppipe[0][0], final, 1) != 1) {
    perror("Unable to read finalTeam1 from group pipe");
    exit(0);
  }
  finalTeam1 = atoi(final);

  //finalTeam2 = read from grp[1][0]
  if (read(grppipe[1][0], final, 1) != 1) {
    perror("Unable to read finalTeam2 from group pipe");
    exit(0);
  }
  finalTeam2 = atoi(final);
}
