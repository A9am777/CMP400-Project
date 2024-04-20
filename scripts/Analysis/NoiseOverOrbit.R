library(readr)
library(latex2exp)

# read the table
scriptsDir="C:/Users/Adam/source/repos/uni/CMP400-Project/scripts/Analysis/"
data <- read_csv(paste(scriptsDir, "../../data/GTX1050Ti/Orbit Noise/SpatialNoScene.csv"))

png(file=paste(scriptsDir, "OrbitErrorNoScene.png"), 
width = 1024, 
height = 1024, 
pointsize = 24)

# plot the table
table <- plot(data$Progress, data$"RMS",
col=c("black"),
main = "Percieved Error With Orbit \n (No Scene)",
ylab="RMSE",
xlab="Location (°)",
pch=3)

# determine non-linear regression
y=data$"RMS"
x=data$Progress
initialvalues <- c(a=0.00001, b=2*pi/360, c=0, d=0.00001)
regression <- nls(y ~ a * sin(b*x + c) + d,
           algorithm = "port",
           start = initialvalues,
           control = nls.control(maxiter = 1000))

# display and output summary of the fit
summ = summary(regression)
capture.output(summ, file = paste(scriptsDir, "OrbitErrorNoScene.txt"))

# plot predicted line
prediction <- predict(regression)
lines(data$Progress, prediction, col='blue')

# display predicted legend
legend("topleft", 
legend=c(TeX(r'($y\sim a\cdot\sin(b\cdot x+c)+d$)', bold=FALSE)), 
col=c("blue"), lty=1, cex=0.75)

dev.off()