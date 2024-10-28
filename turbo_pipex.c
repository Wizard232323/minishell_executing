//
//
// EK - Rosalie --->>> MINI SHELL aka curse-shell
//

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define WRITE 1
#define READ 0

typedef struct s_cmd {
    bool    append;          //true si la redirection de sortie est en mode append
    char    *inout[2];       //nom des fihier de redirection d'entre et de sortie, NULL si pas de redirection
    int     ac;
    char    **av;
    pid_t   pid;               //vide au debut doit etre set au pid de la cmd actuelle
}   t_cmd;

typedef struct s_rosa {
    t_cmd   **cmds;       // tableau de toutes les commandes a executer
    int     nb_cmd;    // nombre de commqndes a executer
    int     exit_code;   // exit code de la dernier commande;
    char    **env;       // environnement pour le path
}   t_rosa;

int ft_close(int fd)
{
    if (fd > 2)
        return (close(fd));
    return (EXIT_SUCCESS);
}

void closepipe(int *pipefd)
{   
    ft_close(pipefd[READ]);
    ft_close(pipefd[WRITE]);
}
int fail_fork(int *pipefd)
{
    perror("fail to fork");
    if (pipefd)
        closepipe(pipefd);
    exit(1);
}

void    ft_free(char **tab)
{
    int i;
    int j;

    i = 0;
    j = 0;
    while(tab[i])
    {
        free(tab[i]);
        i++;
    }
    free(tab);
}


size_t	ft_strlen(const char *s)
{
	size_t	i;

	i = 0;
	while (s[i] != 0)
		i++;
	return (i);
}

char	*ft_strjoin(char const *s1, char const *s2)
{
	size_t	i;
	size_t	j;
	size_t	k;
	size_t	l;
	char	*s3;

	i = ft_strlen(s1);
	j = ft_strlen(s2);
	k = 0;
	l = 0;
	s3 = (char *)malloc((i + j) + 1);
	if (!s3)
		return (NULL);
	while (s1[k])
	{
		s3[k] = s1[k];
		k++;
	}
	while (s2[l])
	{
		s3[k + l] = s2[l];
		l++;
	}
	s3[k + l] = '\0';
	return (s3);
}



char	*ft_substr(char const *s, unsigned int start, size_t len)
{
	size_t	i;
	size_t	j;
	char	*s2;

	i = 0;
	j = ft_strlen(s);
	if (start >= j)
		start = j;
	if (len >= j - start)
		len = j - start;
	s2 = (char *)malloc(len + 1);
	if (!s2)
		return (NULL);
	while (s[start] && i < len)
	{
		s2[i] = s[start];
		i++;
		start++;
	}
	s2[i] = '\0';
	return (s2);
}

int	nbword(char *s, char c)
{
	int	i;
	int	count;

	i = 1;
	count = 0;
	if (!*s)
		return (0);
	if (s[0] != c)
		count++;
	while (s[i])
	{
		if (s[i - 1] == c && s[i] != c)
			count++;
		i++;
	}
	return (count);
}

char	**ft_split(char const *s, char c)
{
	char	**rep;
	int		word;
	int		i;
	int		j;
	int		k;

    j = 0;
	i = 0;
	k = 0;
	word = nbword((char *)s, c);
	rep = (char **)malloc(sizeof (char *) * (word + 1));
	if (!rep)
		return (NULL);
	rep[word] = NULL;
	while (k < word)
	{
		while (s[i] == c && s[i] != 0)
			i++;
		j = i;
		while (s[i] != c && s[i] != 0)
			i++;
		rep[k] = ft_substr((char *)s, j, i - j);
		k++;
	}
	return (rep);
}

char *find_path(char **envp, char *to_find)
{
    int i; 
    int j;
    
    i = 0;
    while (envp[i])
    {
        j = 0;
        
        while (envp[i][j] == to_find[j])
        {
            j++;
            if (!to_find[j])
                return(&envp[i][j] + 5);
        }
        i++;
    }
    return (0);
}

char *get_valid_path(char *cmd, char **envp)
{

    char *path_line;
    char **array_env;
    char *tmp;
    char *path;
    int i;
    int j;

    i = 0;
    j = 0;
    if (cmd == NULL)
        return (NULL);
    if (access(cmd, X_OK) == 0)
        return (strdup(cmd));
    path_line = find_path(envp, "PATH=");
    array_env = ft_split(path_line, ':');

    while(array_env[i])
    {
        tmp = ft_strjoin(array_env[i], "/");
        path = ft_strjoin(tmp, cmd);

        free(tmp);

        if (access(path, X_OK) == 0)
		{
            ft_free(array_env);
		    return (path);
		}
        free(path);
        i++;
    }
    ft_free(array_env);
    return (NULL);
}
        
int handle_redirection(t_cmd *cmd, t_rosa *rosa)
{
    int fd1;
    int fd2;

    if (cmd->inout[0] != NULL)
    {
        fd1 = open(cmd->inout[0], O_RDONLY);
        if (fd1 < 0)
        {
            write(2, "Files error\n", 12);
            close(fd1);
            return (1);
        }
        dup2(fd1, STDIN_FILENO);
    }
    if (cmd->inout[1] != NULL)
    {
        if (cmd->append)
            fd2 = open(cmd->inout[1], O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
        else 
            fd2 = open(cmd->inout[1], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        if (fd2 < 0)
        {
            write(2, "Files error\n", 12);
            close(fd2);
            return (1);
        }
        dup2(fd2, STDOUT_FILENO);
    }
    return (0);
}
int execute(t_cmd *cmd, t_rosa *rosa)
{
    char *valid_path;

    valid_path = get_valid_path(cmd->av[0], rosa->env);
    if (valid_path == NULL)
    {
        write(2, "command not found\n", 18);
        exit(1);
    }

    execve(valid_path, cmd->av, rosa->env);
    perror("execve");
    free(valid_path);
    exit(1);
}

int last_cmd(int fdin, t_cmd *cmd, t_rosa *rosa)
{

    cmd->pid = fork();
    if (cmd->pid < 0)
        fail_fork(NULL);
    if (cmd->pid == 0)
    {
        dup2(fdin, STDIN_FILENO);
        ft_close(fdin);
        handle_redirection(cmd, rosa);
        execute(cmd, rosa);
    }
    ft_close(fdin);
    return(0);
}

int child_process(int fdin, t_cmd *cmd, t_rosa *rosa)
{
    int pipefd[2];

    if (pipe(pipefd) == -1) 
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    cmd->pid = fork();
    if (cmd->pid < 0)
        fail_fork(pipefd);    
    if (cmd->pid == 0)
    { 
        dup2(fdin, STDIN_FILENO);
        dup2(pipefd[WRITE], STDOUT_FILENO);
        ft_close(pipefd[READ]);
        handle_redirection(cmd, rosa);
        ft_close(fdin);
        execute(cmd, rosa);
        perror("cmd error :");
    }
    ft_close(fdin);
    ft_close(pipefd[WRITE]);
    return (pipefd[READ]);
}

int minishell(t_rosa *rosa)
{
    int i;
    t_cmd cmd;
    int fdin;

    fdin = STDIN_FILENO;
    i = 0;


    while (i < rosa->nb_cmd - 1)
        fdin = child_process(fdin, rosa->cmds[i++], rosa);
    last_cmd(fdin, rosa->cmds[i], rosa);

    i = 0;
    while(i < rosa->nb_cmd)
    {
        wait(NULL);
        i++;
    }
    return (0);
}

int main (int argc, char **argv, char **envp)
{
    t_rosa rosa;
    t_cmd cmds[3];  // Trois commandes à exécuter
    int i;

    // Première commande: "ls -la"
    cmds[0].append = false;       // Pas d'append, simple redirection
    cmds[0].inout[0] = "test";      // Pas de fichier d'entrée
    cmds[0].inout[1] = "tfsdakfsdafjks";      // Pas de fichier de sortie
    cmds[0].ac = 3;               // 2 arguments
    cmds[0].av = (char *[]){"cat", NULL};  // Tableau des arguments
    cmds[0].pid = 0;              // PID vide au départs

    // Deuxième commande: "grep minishell"
    cmds[1].append = false;       // Pas d'append, simple redirection
    cmds[1].inout[0] = NULL;      // Pas de fichier d'entrée
    cmds[1].inout[1] = NULL;      // Pas de fichier de sortie
    cmds[1].ac = 2;               // 2 arguments
    cmds[1].av = (char *[]){"grep", "minishell", NULL};  // Tableau des arguments
    cmds[1].pid = 0;              // PID vide au départ

    // Troisième commande: "wc -l > output.txt"
    cmds[2].append = true;       // Pas d'append, redirection simple
    cmds[2].inout[0] = NULL;      // Pas de fichier d'entrée
    cmds[2].inout[1] = "newnew23.txt";  // Fichier de sortie
    cmds[2].ac = 2;               // 2 arguments
    cmds[2].av = (char *[]){"rev", NULL};  // Tableau des arguments
    cmds[2].pid = 0;              // PID vide au départ

    // Remplissage de la structure rosa
    rosa.cmds = (t_cmd *[]){&cmds[0], &cmds[1], &cmds[2], NULL};            // Tableau des commandes
    rosa.exit_code = 0;          // Code de sortie initial
    rosa.env = envp;          // Environnement pour les commandes

    //printf("cmds: %d \n", cmds);

    rosa.nb_cmd = 3;
    minishell(&rosa);

    return (0);
}