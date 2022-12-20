@echo off

:: Confirm initialisation is wanted
:Start
if exist ".git" (
  	set /p ans="[31mExisting .git file will be removed. Type yes to continue: [0m"

	if NOT "%ans%" == "yes" (
		goto :Start
	)

)

:: delete any existing .git files
RMDIR /s /q ".git"

:: Create new git repository
git init -b main

:: Add the files in your new local respository
git add .

:: Commit the files that you have stage in your local repository
git commit -m "Project Initialisation"

:: Add the URL
set /p url="[31mPast repository URL: [0m"
git remote add origin %url%

:: Verify repository
git remote -v

:: Push changes to github
git push origin main

echo [32mProject succesfully pushed to git hub[0m